#include "vm_local.h"
#include "vm_vr.h"
#include "../vrcommon/vr_shared.h"

void VM_VRInit( void ) {
	Cvar_Get( "vm_forceNative", "0", 0 );
}

/*
==============
VM_VRSelectModule

Owns module selection for the VR engine.
Phase 1: prefer a VR-aware QVM from the search path (uniform ladder).
vm_forceNative 1 is the kill-switch that restores DLL-only loading.
Phase 2: native DLL fallback, or fail. A plain (non-VR) QVM never loads:
it would replace VR-specific function implementations with flatscreen ones.
==============
*/
qboolean VM_VRSelectModule( vm_t *vm, vmHeader_t **header ) {
	char filename[MAX_OSPATH];
	void *startSearch;
	const char *name = vm->name;
	const vmIndex_t index = vm->index;
	const syscall_t systemCall = vm->systemCall;
	const dllSyscall_t dllSyscall = vm->dllSyscall;
	const int privateFlag = vm->privateFlag;

	*header = NULL;

	if ( Cvar_VariableIntegerValue( "vm_forceNative" ) == 0 ) {
		startSearch = NULL;
		while ( FS_FindVM( &startSearch, filename, sizeof( filename ), name, qtrue ) == VMI_COMPILED ) {
			int minor = 0;
			int major = FS_GetVMVRAPIVersion( name, startSearch, &minor );
			const char *pakName = FS_VMSearchPathName( startSearch );
			if ( major == 0 ) {
				// a plain non-VR QVM: fall through to the native module
				Com_Printf( "%s.qvm in %s is not VR-aware; using native %s\n", name, pakName, name );
			} else if ( major == VR_API_MAJOR && minor <= VR_API_MINOR ) {
				Com_Printf( "%s: loading VR-aware QVM (VR API %d.%d) from %s\n", name, major, minor, pakName );
				vm->searchPath = startSearch;
				if ( ( *header = VM_LoadQVM( vm, qtrue ) ) != NULL ) {
					vm->vrSentinel = qtrue;
					return qtrue;
				}
				// VM_LoadQVM wipes the vm slot on failure; restore its
				// identity and keep walking
				vm->name = name;
				vm->index = index;
				vm->systemCall = systemCall;
				vm->dllSyscall = dllSyscall;
				vm->privateFlag = privateFlag;
			} else {
				// a VR module the engine can't satisfy: drop, so the native
				// module (a different game) never runs silently in its place
				Com_Error( ERR_DROP, "%s.qvm (from %s): VR API incompatible: engine %d.%d, mod %d.%d",
					name, pakName, VR_API_MAJOR, VR_API_MINOR, major, minor );
			}
		}
	}

	// Phase 2: native DLL fallback (previous behavior)
	startSearch = NULL;
	while ( FS_FindVM( &startSearch, filename, sizeof( filename ), name, qfalse ) == VMI_NATIVE ) {
		Com_Printf( "Try loading dll file %s\n", filename );
		vm->dllHandle = Sys_LoadGameDll( filename, &vm->entryPoint, vm->dllSyscall );
		if ( vm->dllHandle ) {
			vm->privateFlag = 0; // allow reading private cvars
			vm->dataAlloc = ~0U;
			vm->dataMask = ~0U;
			vm->dataBase = 0;
			return qtrue;
		}
		Com_Printf( "Failed loading dll, trying next\n" );
	}
	return qfalse;
}

void VM_VRModuleUnloaded( vm_t *vm ) {
	if ( vm->vrShared ) {
		VR_SharedModuleUnloaded( vm->vrWriter );
	}
	// module must re-register during INIT; vrSentinel is preserved (same image)
	vm->vrShared = NULL;
}

/*
Sync only around the outermost call: the mirror is shared-pointer state,
so a nested same-VM call (e.g. trap_UpdateScreen -> UI_REFRESH) must see
the mirror as-is (fresh engine state plus the outer call's own writes).
Re-syncing on nested entry would clobber uncommitted writer-block writes,
and re-committing on nested exit would push stale values back to the engine.
*/
void VM_VRCallEnter( vm_t *vm ) {
	if ( vm->vrShared && vm->callLevel == 1 )
		VR_SharedSyncIn( (vr_shared_t *)vm->vrShared, vm->vrStructSize );
}

void VM_VRCallLeave( vm_t *vm ) {
	if ( vm->vrShared && vm->callLevel == 1 )
		VR_SharedSyncOut( (vr_shared_t *)vm->vrShared, vm->vrWriter, vm->vrStructSize );
}

/*
==============
VM_RegisterVRShared

Module handed us its vr_shared_t mirror. Validate the handshake, translate
the address (QVM: offset into dataBase; DLL: host pointer), and start syncing.
==============
*/
void VM_RegisterVRShared( vm_t *vm, int writer, intptr_t vmAddr, int structSize, int apiVersion ) {
	const char *pakName = FS_VMSearchPathName( vm->searchPath );
	// The version gate already ran at load, so the handshake is trusted. structSize
	// comes from (possibly hostile) module memory, so clamp it to [0,sizeof] ONCE
	// here and drive every sync from the stored value - never re-read it from the
	// module (TOCTOU). The engine only ever touches structSize bytes of the block.
	(void)apiVersion;
	if ( structSize < 0 )
		structSize = 0;
	if ( structSize > (int)sizeof( vr_shared_t ) )
		structSize = (int)sizeof( vr_shared_t );
	vm->vrStructSize = structSize;
	if ( vm->entryPoint ) {
		// native DLL: shared address space
		vm->vrShared = (struct vr_shared_s *)vmAddr;
	} else {
		unsigned dest = (unsigned)vmAddr;
		if ( dest == 0 || ( dest & 3 ) != 0 ) {
			Com_Error( ERR_DROP, "%s: VR shared block address invalid", vm->name );
		}
		if ( dest > (unsigned)vm->dataMask ||
			 (unsigned)structSize > (unsigned)vm->dataMask + 1 - dest ) {
			Com_Error( ERR_DROP, "%s (from %s): VR shared block out of VM bounds", vm->name, pakName );
		}
		vm->vrShared = (struct vr_shared_s *)( vm->dataBase + dest );
	}
	vm->vrWriter = writer;
	Com_Printf( "%s: VR shared state registered (VR API %d.%d, %s)\n",
		vm->name, VR_API_MAJOR, VR_API_MINOR, vm->entryPoint ? "native" : "QVM" );
	// module registered mid-call; give it fresh state immediately so init code
	// after the register call reads live values
	VR_SharedSyncIn( (vr_shared_t *)vm->vrShared, vm->vrStructSize );
}

qboolean VM_VRSentinel( vm_t *vm ) {
	return vm ? vm->vrSentinel : qfalse;
}

qboolean VM_VRRegistered( vm_t *vm ) {
	return ( vm && vm->vrShared ) ? qtrue : qfalse;
}
