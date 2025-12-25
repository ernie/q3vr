#ifndef __VR_GL_TYPES
#define __VR_GL_TYPES

// vr_types.h includes SDL_opengl.h and defines XR_USE_GRAPHICS_API_OPENGL
// for non-Vulkan builds, giving us access to OpenGL-specific OpenXR types
#include "../vrcommon/vr_types.h"

// OpenGL graphics requirements from OpenXR
typedef struct {
	XrGraphicsRequirementsOpenGLKHR requirements;
} VR_GL_GraphicsRequirements;

// OpenGL-specific swapchain info
typedef struct {
	XrSwapchain swapchain;
	int64_t swapchainFormat;
	uint32_t imageCount;
	uint32_t* images;           // GLuint texture handles from XR
	uint32_t virtualScreenImage;

	int width;
	int height;
} VR_GL_SwapchainInfo;

// Alias for use in common VR code
typedef VR_GL_SwapchainInfo VR_SwapchainInfo;

// Concrete implementation of VR_SwapchainInfos for OpenGL
struct VR_SwapchainInfos_s {
	uint32_t viewCount;
	VR_GL_SwapchainInfo color;
	VR_GL_SwapchainInfo depth;
	VR_GL_SwapchainInfo screenOverlay;     // Single-layer texture for 2D screen overlays
	GLuint* framebuffers;
	GLuint** eyeFramebuffers;              // separate FBOs with bound only single eye image
	GLuint virtualScreenFramebuffer;
	GLuint screenOverlayFramebuffer;       // FBO for screen overlay rendering
};

#endif
