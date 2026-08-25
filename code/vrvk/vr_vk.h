#ifndef __VR_VULKAN_H
#define __VR_VULKAN_H

#include <vulkan/vulkan.h>
#include "../vrcommon/vr_types.h"
#include "../vrcommon/vr_backend.h"
#include "vr_vk_types.h"

// Get the Vulkan graphics extension name for OpenXR instance creation
const char* VR_VK_GetGraphicsExtensionName(void);

// Get Vulkan graphics requirements from OpenXR
// Must be called after VR_Init() creates the XR instance
XrResult VR_VK_GetGraphicsRequirements(XrInstance instance, XrSystemId systemId,
                                        VR_VK_GraphicsRequirements* requirements);

// Print graphics requirements debug info
void VR_VK_PrintGraphicsRequirements(const VR_VK_GraphicsRequirements* requirements);

// Vulkan state managed by VR layer for OpenXR integration
// This is created before renderervk initialization and passed to it
typedef struct {
    VkInstance instance;
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    VkQueue queue;
    uint32_t queueFamilyIndex;

    // Graphics requirements from OpenXR
    uint32_t minApiVersionSupported;
    uint32_t maxApiVersionSupported;

    // Device properties (cached for use by renderer)
    VkPhysicalDeviceProperties deviceProperties;
    VkPhysicalDeviceMemoryProperties memoryProperties;
    VkPhysicalDeviceFeatures deviceFeatures;

    // Multiview support (required for stereo rendering)
    VR_Bool multiviewSupported;
    uint32_t maxMultiviewViewCount;

    // XR_FB_color_space: Rec709 declared to the headset runtime
    VR_Bool xrColorManaged;

    // VK_EXT_swapchain_colorspace enabled on the instance (needed for HDR desktop mirror)
    VkBool32 swapchainColorspaceEnabled;
} VR_VulkanState;

// Global VR Vulkan state
extern VR_VulkanState vr_vk;

// Initialization functions: called in sequence during VR startup
// These implement the XR_KHR_vulkan_enable2 workflow

// Check graphics requirements before creating Vulkan instance
XrResult VR_Vulkan_CheckRequirements(XrInstance xrInstance, XrSystemId systemId);

// Create VkInstance (runtime adds required extensions via xrCreateVulkanInstanceKHR)
XrResult VR_Vulkan_CreateInstance(XrInstance xrInstance, XrSystemId systemId);

// Get the physical device that OpenXR requires
XrResult VR_Vulkan_GetPhysicalDevice(XrInstance xrInstance, XrSystemId systemId);

// Create VkDevice with VK_KHR_multiview (runtime adds required extensions via xrCreateVulkanDeviceKHR)
XrResult VR_Vulkan_CreateDevice(XrInstance xrInstance, XrSystemId systemId);

// Cleanup
void VR_Vulkan_Shutdown(void);

// Helper to check if Vulkan VR is initialized
VR_Bool VR_Vulkan_IsInitialized(void);

// Minimal device info for renderer initialization (pull model)
// This is what the renderer needs from the VR layer's XR_KHR_vulkan_enable2 workflow
typedef struct {
    VkInstance instance;
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    VkQueue queue;
    uint32_t queueFamilyIndex;
    VkBool32 swapchainColorspaceEnabled;  // VK_EXT_swapchain_colorspace enabled on the instance
} VR_VulkanDeviceInfo;

// Get the XR-created Vulkan device for renderer initialization
// Returns NULL if VR Vulkan is not yet initialized
const VR_VulkanDeviceInfo* VR_Vulkan_GetDeviceInfo(void);

// Swapchain info for renderer initialization (pull model)
typedef struct {
    // Color (multiview stereo, arraySize=2)
    VkFormat colorFormat;
    uint32_t colorWidth, colorHeight, colorArraySize, colorImageCount;
    VkImage* colorImages;          // NOT owned: from OpenXR
} VR_VulkanSwapchainInfo;

// Get the XR swapchain info for renderer initialization
// Returns NULL if swapchains are not yet created
const VR_VulkanSwapchainInfo* VR_Vulkan_GetSwapchainInfo(void);

// XR Swapchain types are now defined in vr_vk_types.h:
// - VR_VK_SwapchainInfo - minimal VR layer swapchain info (XrSwapchain + VkImages)
// - VR_SwapchainInfos_s - concrete VR_SwapchainInfos for Vulkan
//
// VkImageViews, VkFramebuffers, virtual screen resources, etc. are owned by
// the renderer and stored in VkXrResources (see renderervk/vk.h)

// Swapchain format selection
VkFormat VR_Vulkan_SelectColorFormat(const int64_t* formats, uint32_t count);
VkFormat VR_Vulkan_SelectDepthFormat(const int64_t* formats, uint32_t count);

// Swapchain creation and management
XrResult VR_Vulkan_CreateSwapchain(XrSession session, VkFormat format,
                                    uint32_t width, uint32_t height,
                                    uint32_t arraySize, XrSwapchainUsageFlags usage,
                                    XrSwapchain* swapchain);

// Create swapchain with format list (XR_KHR_vulkan_swapchain_format_list)
// This allows specifying which formats will be used for image views,
// helping the runtime avoid adding unnecessary usage flags like STORAGE_BIT
XrResult VR_Vulkan_CreateSwapchainWithFormatList(XrSession session, VkFormat format,
                                                  uint32_t width, uint32_t height,
                                                  uint32_t arraySize, XrSwapchainUsageFlags usage,
                                                  const VkFormat* viewFormats, uint32_t viewFormatCount,
                                                  XrSwapchain* swapchain);

XrResult VR_Vulkan_GetSwapchainImages(XrSwapchain swapchain,
                                       VkImage** images, uint32_t* imageCount);

// VkImageView/VkFramebuffer creation in renderer (see vk_create_xr_image_views in vk.c)

// ============================================================================
// vr_backend_t implementation for Vulkan (see vrcommon/vr_backend.h)
// ============================================================================

// vr_graphics.h roles
const char* VRVK_GetExtensionName( void );
XrResult    VRVK_GetRequirements( XrInstance instance, XrSystemId systemId );
void        VRVK_PrintRequirements( void );
void        VRVK_GraphicsInit( XrInstance instance, XrSystemId systemId );
void        VRVK_GraphicsShutdown( void );
void        VRVK_InvalidateFunctionPointers( void );
XrResult    VRVK_CreateSession( XrInstance instance, XrSystemId systemId, XrSession *session );

// vr_renderer.h roles
void        VRVK_GetResolution( VR_Engine *engine, int *pWidth, int *pHeight );
void        VRVK_InitRenderer( VR_Engine *engine );
void        VRVK_DestroyRenderer( VR_Engine *engine );
void        VRVK_ProcessFrame( VR_Engine *engine );
void        VRVK_RestoreState( VR_Engine *engine );
qboolean    VRVK_SubmitLoadingFrame( VR_Engine *engine );

// Ends a frame the other backend began, after a mid-frame renderer switch.
void        VRVK_FinishFrame( VR_Engine *engine );

const vr_backend_t* VRVK_GetBackend( void );

#endif // __VR_VULKAN_H
