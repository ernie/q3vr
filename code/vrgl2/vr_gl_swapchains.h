#ifndef __VR_GL_SWAPCHAINS
#define __VR_GL_SWAPCHAINS

#include "../qcommon/q_shared.h"
#include "../vrcommon/vr_types.h"
#include "../vrcommon/vr_swapchains.h"  // Common OpenXR utilities
#include "vr_gl_types.h"

// Common utilities are now in vrcommon/vr_swapchains.h:
// - XrDesktopViewConfiguration enum
// - VR_GetSupersamplingFactor()
// - VR_GetRecommendedResolution()
// - VR_GetSupersampledResolution()
// - VR_GetBestViewConfiguration()
// - VR_GetViewConfigurationViews()
// - VR_GetSwapchainFormats()
// - VR_HasFormatInList()

VR_SwapchainInfos* VR_CreateSwapchains(XrInstance instance, XrSystemId systemId, XrSession session);
void VR_DestroySwapchains(VR_SwapchainInfos** swapchains);

void VR_Swapchains_BindFramebuffers(VR_SwapchainInfos* swapchains, uint32_t swapchainColorIndex, uint32_t swapchainDepthIndex);
void VR_Swapchains_BlitXRToMainFbo(VR_SwapchainInfos* swapchains, uint32_t swapchainImageIndex, XrDesktopViewConfiguration viewConfig, qboolean useVirtualScreen);
void VR_Swapchains_BlitXRToVirtualScreen(VR_SwapchainInfos* swapchains, uint32_t swapchainImageIndex);

void VR_Swapchains_Acquire(VR_SwapchainInfos* swapchains, uint32_t* colorIndex, uint32_t* depthIndex);
void VR_Swapchains_Release(VR_SwapchainInfos* swapchains);

#endif
