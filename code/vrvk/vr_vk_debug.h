#ifndef __VR_VK_DEBUG
#define __VR_VK_DEBUG

#define VR_ENABLE_VK_DEBUG 0
#if VR_ENABLE_VK_DEBUG
#define VR_ENABLE_VK_DEBUG_VERBOSE 0
#else
#define VR_ENABLE_VK_DEBUG_VERBOSE 0
#endif

//
// VK Debug
//
void VR_VK_RegisterDebugCallbackIfEnabled(void);

#endif
