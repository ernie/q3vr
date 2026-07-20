#include "vr_backend.h"
#include "vr_graphics.h"
#include "vr_renderer.h"

static const vr_backend_t *vr_backendActive = NULL;

static qboolean vr_frameInFlight = qfalse;

void VR_SetBackend( const vr_backend_t *backend )
{
	vr_backendActive = backend;
}

void VR_SetFrameInFlight( qboolean inFlight )
{
	vr_frameInFlight = inFlight;
}

qboolean VR_FrameInFlight( void )
{
	return vr_frameInFlight;
}

const vr_backend_t* VR_GetActiveBackend( void )
{
	if ( !vr_backendActive ) {
		Com_Error( ERR_FATAL, "VR_GetActiveBackend: no backend selected" );
	}
	return vr_backendActive;
}

int VR_Backend_GetStencilBits( void )
{
	return VR_GetActiveBackend()->GetStencilBits();
}

// ---- vr_graphics.h dispatchers (names preserved for existing callers) ----

const char* VR_Graphics_GetExtensionName( void )
{ return VR_GetActiveBackend()->GetExtensionName(); }

XrResult VR_Graphics_GetRequirements( XrInstance instance, XrSystemId systemId )
{ return VR_GetActiveBackend()->GetRequirements( instance, systemId ); }

void VR_Graphics_PrintRequirements( void )
{ VR_GetActiveBackend()->PrintRequirements(); }

void VR_Graphics_Init( XrInstance instance, XrSystemId systemId )
{ VR_GetActiveBackend()->GraphicsInit( instance, systemId ); }

void VR_Graphics_Shutdown( void )
{ VR_GetActiveBackend()->GraphicsShutdown(); }

void VR_Graphics_InvalidateFunctionPointers( void )
{ VR_GetActiveBackend()->InvalidateFunctionPointers(); }

XrResult VR_Graphics_CreateSession( XrInstance instance, XrSystemId systemId, XrSession *session )
{ return VR_GetActiveBackend()->CreateSession( instance, systemId, session ); }

// ---- vr_renderer.h dispatchers ----

void VR_GetResolution( VR_Engine *engine, int *pWidth, int *pHeight )
{ VR_GetActiveBackend()->GetResolution( engine, pWidth, pHeight ); }

void VR_InitRenderer( VR_Engine *engine )
{ VR_GetActiveBackend()->InitRenderer( engine ); }

void VR_DestroyRenderer( VR_Engine *engine )
{ VR_GetActiveBackend()->DestroyRenderer( engine ); }

void VR_ProcessFrame( VR_Engine *engine )
{ VR_GetActiveBackend()->ProcessFrame( engine ); }

void VR_Renderer_RestoreState( VR_Engine *engine )
{ VR_GetActiveBackend()->RestoreState( engine ); }

qboolean VR_Renderer_SubmitLoadingFrame( VR_Engine *engine )
{ return VR_GetActiveBackend()->SubmitLoadingFrame( engine ); }
