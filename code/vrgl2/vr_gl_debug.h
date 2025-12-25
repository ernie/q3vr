#ifndef __VR_GL_DEBUG
#define __VR_GL_DEBUG

#define VR_ENABLE_GL_DEBUG 0
#if VR_ENABLE_GL_DEBUG
#define VR_ENABLE_GL_DEBUG_VERBOSE 0
#else
#define VR_ENABLE_GL_DEBUG_VERBOSE 0
#endif

//
// GL Debug
//
void VR_GL_RegisterDebugLogSinkIfEnabled(void);

#endif
