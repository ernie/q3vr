#ifndef __VR_GL_H
#define __VR_GL_H

#include "../vrcommon/vr_types.h"
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

#endif
