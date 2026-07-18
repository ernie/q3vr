#include "vm_local.h"
#include "vm_vr.h"
#include "../vrcommon/vr_shared.h"

/*
==============
VM_VRLoadNative

Walk the search paths for the native module; used by vm_* 0 and the
no-QVM fallback.
==============
*/
static qboolean VM_VRLoadNative( vm_t *vm ) {
	char filename[MAX_OSPATH];
	void *startSearch = NULL;

	while ( FS_FindVM( &startSearch, filename, sizeof( filename ), vm->name, qfalse ) == VMI_NATIVE ) {
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

/*
==============
VM_VRSelectModule

Module selection for the VR engine. vm_* 0 wants native first (stock);
otherwise prefer a VR-aware QVM with native as fallback. qvmOnly (pure
server) forbids native: bytecode or fail. A plain non-VR QVM never
loads - it would swap VR functions for flatscreen ones.
==============
*/
qboolean VM_VRSelectModule( vm_t *vm, vmInterpret_t *interpret, qboolean qvmOnly, vmHeader_t **header ) {
	char filename[MAX_OSPATH];
	void *startSearch;
	qboolean triedNative = qfalse;
	const char *name = vm->name;
	const vmIndex_t index = vm->index;
	const syscall_t systemCall = vm->systemCall;
	const dllSyscall_t dllSyscall = vm->dllSyscall;
	const int privateFlag = vm->privateFlag;

	*header = NULL;

	// vm_* 0: honor native (pure servers excepted)
	if ( *interpret == VMI_NATIVE && !qvmOnly ) {
		if ( VM_VRLoadNative( vm ) )
			return qtrue;
		// stock fallthrough: no native, run bytecode
		triedNative = qtrue;
	}

	startSearch = NULL;
	while ( FS_FindVM( &startSearch, filename, sizeof( filename ), name, qtrue ) == VMI_COMPILED ) {
		int minor = 0;
		int major = FS_GetVMVRAPIVersion( name, startSearch, &minor );
		const char *pakName = FS_VMSearchPathName( startSearch );
		if ( major == 0 ) {
			// plain non-VR QVM: fall through to native
			Com_Printf( "%s.qvm in %s is not VR-aware; skipping\n", name, pakName );
		} else if ( major == VR_API_MAJOR && minor <= VR_API_MINOR ) {
			Com_Printf( "%s: loading VR-aware QVM (VR API %d.%d) from %s\n", name, major, minor, pakName );
			vm->searchPath = startSearch;
			if ( ( *header = VM_LoadQVM( vm, qtrue ) ) != NULL ) {
				vm->vrSentinel = qtrue;
				// a QVM can't run native; execute under the JIT
				if ( *interpret == VMI_NATIVE )
					*interpret = VMI_COMPILED;
				return qtrue;
			}
			// VM_LoadQVM wipes the slot on failure; restore identity and keep walking
			vm->name = name;
			vm->index = index;
			vm->systemCall = systemCall;
			vm->dllSyscall = dllSyscall;
			vm->privateFlag = privateFlag;
		} else {
			// VR module the engine can't satisfy: drop, so native
			// never silently runs a different game in its place
			Com_Error( ERR_DROP, "%s.qvm (from %s): VR API incompatible: engine %d.%d, mod %d.%d",
				name, pakName, VR_API_MAJOR, VR_API_MINOR, major, minor );
		}
	}

	// native fallback, unless the server demands bytecode
	if ( !qvmOnly && !triedNative && VM_VRLoadNative( vm ) )
		return qtrue;
	return qfalse;
}

/*
==============
VM_VRLoadQVMFile

Read the QVM from the ladder-pinned search path, not the FS-priority winner.
==============
*/
int VM_VRLoadQVMFile( vm_t *vm, const char *filename, void **buffer ) {
	if ( vm->searchPath )
		return (int)FS_ReadFileDir( filename, vm->searchPath, qfalse, buffer );
	return (int)FS_ReadFile( filename, buffer );
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
void VM_RegisterVRShared( vm_t *vm, int writer, intptr_t vmAddr, int structSize, int apiMajor, int apiMinor ) {
	const char *pakName = FS_VMSearchPathName( vm->searchPath );
	// Registration is the module's version advertisement: the major.minor pair
	// it was compiled against. Enforce the same contract as the load gate - run
	// a module whose major matches and whose minor the engine can meet. For
	// native modules, which never pass the QVM sentinel scan, this is the only
	// version check; for QVMs it holds the compiled-in pair to the sentinel's word.
	if ( apiMajor != VR_API_MAJOR || apiMinor > VR_API_MINOR ) {
		Com_Error( ERR_DROP, "%s: VR API incompatible: engine %d.%d, mod %d.%d",
			vm->name, VR_API_MAJOR, VR_API_MINOR, apiMajor, apiMinor );
	}
	// structSize comes from (possibly hostile) module memory, so clamp it to
	// [0,sizeof] ONCE here and drive every sync from the stored value - never
	// re-read it from the module (TOCTOU). The engine only ever touches
	// structSize bytes of the block.
	if ( structSize < 0 )
		structSize = 0;
	if ( structSize > (int)sizeof( vr_shared_t ) )
		structSize = (int)sizeof( vr_shared_t );
	vm->vrStructSize = structSize;
	if ( vm->entryPoint ) {
		// native DLL: shared address space
		if ( vmAddr == 0 ) {
			Com_Error( ERR_DROP, "%s: VR shared block address invalid", vm->name );
		}
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
	Com_Printf( "%s: VR shared state registered (mod VR API %d.%d, %s)\n",
		vm->name, apiMajor, apiMinor, vm->entryPoint ? "native" : "QVM" );
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
