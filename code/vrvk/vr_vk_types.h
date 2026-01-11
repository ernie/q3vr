#ifndef __VR_VK_TYPES
#define __VR_VK_TYPES

// This header is ONLY for Vulkan-specific VR types.
// It must set up Vulkan graphics API binding for OpenXR.

#include <vulkan/vulkan.h>

// Platform defines for OpenXR (must be before openxr.h)
#if defined(WIN32)
#include "unknwn.h"
#define XR_USE_PLATFORM_WIN32
#elif defined(__ANDROID__)
#define XR_USE_PLATFORM_ANDROID
#else
#include <X11/Xlib.h>
#define XR_USE_PLATFORM_XLIB
#endif

// Vulkan graphics API binding - required for XrGraphicsRequirementsVulkan2KHR, etc.
#define XR_USE_GRAPHICS_API_VULKAN
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>

// Vulkan graphics requirements from OpenXR (XR_KHR_vulkan_enable2)
typedef struct {
	XrGraphicsRequirementsVulkan2KHR requirements;
} VR_VK_GraphicsRequirements;

// Vulkan-specific swapchain info
// VR layer only stores XrSwapchain handles and VkImage references.
// VkImageViews and VkFramebuffers are created by the renderer (see VkXrResources in vk.h)
typedef struct VR_VK_SwapchainInfo_s {
	XrSwapchain swapchain;       // XR swapchain handle (owned by VR layer)
	VkFormat format;             // Chosen format
	uint32_t width;
	uint32_t height;
	uint32_t arraySize;          // 2 for stereo multiview
	uint32_t imageCount;         // Number of swapchain images
	VkImage* images;             // VkImage handles from XR (NOT owned - from OpenXR)
} VR_VK_SwapchainInfo;

// Concrete implementation of VR_SwapchainInfos for Vulkan
// VR layer only owns XrSwapchain handles. All Vulkan resources (VkImageViews,
// VkFramebuffers, virtual screen, etc.) are owned by the renderer (see VkXrResources in vk.h)
struct VR_SwapchainInfos_s {
	uint32_t viewCount;
	VR_VK_SwapchainInfo color;             // 2-layer multiview for stereo
	VR_VK_SwapchainInfo depth;             // 2-layer multiview for stereo
};

#endif
