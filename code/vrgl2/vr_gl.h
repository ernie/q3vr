#ifndef __VR_GL_H
#define __VR_GL_H

#include "../vrcommon/vr_types.h"
#include "../vrcommon/vr_backend.h"
#include "vr_gl_types.h"

// Get the OpenGL graphics extension name for OpenXR
const char* VR_GL_GetGraphicsExtensionName(void);

// Get OpenGL graphics requirements from OpenXR
// Must be called after VR_Init() creates the XR instance
XrResult VR_GL_GetGraphicsRequirements(XrInstance instance, XrSystemId systemId,
                                        VR_GL_GraphicsRequirements* requirements);

// Print graphics requirements debug info
void VR_GL_PrintGraphicsRequirements(const VR_GL_GraphicsRequirements* requirements);

// Initialize OpenGL-specific VR subsystems
void VR_GL_Init(void);

// Shutdown OpenGL-specific VR subsystems
void VR_GL_Shutdown(void);

// ============================================================================
// vr_backend_t implementation for OpenGL (see vrcommon/vr_backend.h)
// ============================================================================

// vr_graphics.h roles
const char* VRGL_GetExtensionName( void );
XrResult    VRGL_GetRequirements( XrInstance instance, XrSystemId systemId );
void        VRGL_PrintRequirements( void );
void        VRGL_GraphicsInit( XrInstance instance, XrSystemId systemId );
void        VRGL_GraphicsShutdown( void );
void        VRGL_InvalidateFunctionPointers( void );
XrResult    VRGL_CreateSession( XrInstance instance, XrSystemId systemId, XrSession *session );

// vr_renderer.h roles
void        VRGL_GetResolution( VR_Engine *engine, int *pWidth, int *pHeight );
void        VRGL_InitRenderer( VR_Engine *engine );
void        VRGL_DestroyRenderer( VR_Engine *engine );
void        VRGL_ProcessFrame( VR_Engine *engine );
void        VRGL_RestoreState( VR_Engine *engine );
qboolean    VRGL_SubmitLoadingFrame( VR_Engine *engine );

// Ends a frame the other backend began, after a mid-frame renderer switch.
void        VRGL_FinishFrame( VR_Engine *engine );

// GL stencil accessor (vr_gl_swapchains.c owns vrgl_stencilBits)
int         VRGL_GetStencilBits( void );

const vr_backend_t* VRGL_GetBackend( void );

#endif
