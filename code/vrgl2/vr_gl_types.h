#ifndef __VR_GL_TYPES
#define __VR_GL_TYPES

// Xlib platform setup must precede vr_types.h so the first
// openxr_platform.h inclusion in GL TUs compiles the GLX binding section.
#if !defined(WIN32) && !defined(__ANDROID__)
#include <X11/Xlib.h>
#include <GL/glx.h>
#define XR_USE_PLATFORM_XLIB
#endif

// vr_types.h gives OpenGL-specific OpenXR types but no real GL types; this
// header's structs use real GLuint/GLenum, so pull them in here too.
#include "../vrcommon/vr_types.h"
#include "SDL_opengl.h"

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
	GLuint nativeDepthTexture;             // Native GL multiview depth texture (replaces XR depth swapchain)
	GLenum depthAttachment;                // GL_DEPTH_STENCIL_ATTACHMENT or GL_DEPTH_ATTACHMENT
	GLuint* framebuffers;
	GLuint** eyeFramebuffers;              // separate FBOs with bound only single eye image
	GLuint virtualScreenFramebuffer;
};

#endif
