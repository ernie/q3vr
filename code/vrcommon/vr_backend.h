#ifndef __VR_BACKEND_H
#define __VR_BACKEND_H

#include <openxr/openxr.h>
#include "../qcommon/q_shared.h"
#include "vr_types.h"

// Runtime dispatch table for the renderer-specific VR backend (vrvk or vrgl2).
// Both are compiled into the client; exactly one is active at a time.
typedef struct vr_backend_s {
	// OpenXR graphics binding (roles from vr_graphics.h)
	const char* (*GetExtensionName)( void );
	XrResult    (*GetRequirements)( XrInstance instance, XrSystemId systemId );
	void        (*PrintRequirements)( void );
	void        (*GraphicsInit)( XrInstance instance, XrSystemId systemId );
	void        (*GraphicsShutdown)( void );
	void        (*InvalidateFunctionPointers)( void );
	XrResult    (*CreateSession)( XrInstance instance, XrSystemId systemId, XrSession *session );
	// Renderer lifecycle (roles from vr_renderer.h)
	void        (*GetResolution)( VR_Engine *engine, int *pWidth, int *pHeight );
	void        (*InitRenderer)( VR_Engine *engine );
	void        (*DestroyRenderer)( VR_Engine *engine );
	void        (*ProcessFrame)( VR_Engine *engine );
	void        (*RestoreState)( VR_Engine *engine );
	qboolean    (*SubmitLoadingFrame)( VR_Engine *engine );
	// Ends a frame the OTHER backend began, after a mid-frame renderer switch.
	// No-op if no frame is in flight.
	void        (*FinishFrame)( VR_Engine *engine );
	// Introspection needed by vrcommon (the concrete VR_SwapchainInfos layout
	// is backend-private; see vr_render_loop.c)
	void        (*GetColorSwapchainDesc)( const VR_SwapchainInfos *sw,
	                                      XrSwapchain *handle, int *width, int *height );
	int         (*GetStencilBits)( void );
} vr_backend_t;

void VR_SetBackend( const vr_backend_t *backend );
const vr_backend_t* VR_GetActiveBackend( void );

// Dispatcher used by refimport wiring (returns 0 on the Vulkan backend)
int VR_Backend_GetStencilBits( void );

// Cross-backend: a mid-frame renderer switch can hand a frame begun by one
// backend to the other. Backends set this in BeginFrame, clear it in
// EndFrame; an in-flight frame always belongs to the active backend.
void     VR_SetFrameInFlight( qboolean inFlight );
qboolean VR_FrameInFlight( void );

#endif
