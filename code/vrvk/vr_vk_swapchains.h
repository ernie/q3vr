/*
 * vr_vk_swapchains.h - Vulkan XR swapchain management
 *
 * VR layer swapchain operations for Vulkan. Creates and manages XrSwapchains
 * and provides access to VkImage handles. VkImageViews and VkFramebuffers
 * are created by the renderer (see VkXrResources in renderervk/vk.h).
 */

#ifndef __VR_VK_SWAPCHAINS
#define __VR_VK_SWAPCHAINS

#include "../qcommon/q_shared.h"
#include "../vrcommon/vr_types.h"
#include "../vrcommon/vr_swapchains.h"  // Common OpenXR utilities
#include "vr_vk_types.h"

// Common utilities are in vrcommon/vr_swapchains.h:
// - XrDesktopViewConfiguration enum
// - VR_GetSupersamplingFactor()
// - VR_GetRecommendedResolution()
// - VR_GetSupersampledResolution()
// - VR_GetBestViewConfiguration()
// - VR_GetViewConfigurationViews()
// - VR_GetSwapchainFormats()
// - VR_HasFormatInList()

// Create XR color swapchain for VR rendering (2-layer for stereo)
// Returns NULL on failure
VR_SwapchainInfos* VR_VK_CreateSwapchains(XrInstance instance, XrSystemId systemId, XrSession session);

// Destroy all swapchains and free resources
void VR_VK_DestroySwapchains(VR_SwapchainInfos** swapchains);

// Acquire color swapchain image for rendering
// Returns index into the swapchain image array
void VR_VK_Swapchains_Acquire(VR_SwapchainInfos* swapchains, uint32_t* colorIndex);

// Release swapchain images after rendering
void VR_VK_Swapchains_Release(VR_SwapchainInfos* swapchains);

// Get color swapchain info for renderer
// The renderer uses this to create VkImageViews and VkFramebuffers
const VR_VK_SwapchainInfo* VR_VK_GetColorSwapchain(const VR_SwapchainInfos* swapchains);

#endif // __VR_VK_SWAPCHAINS
