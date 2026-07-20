/*
 * vr_gl.c - OpenGL-specific VR initialization and extension handling
 *
 * Provides the OpenGL graphics extension name for OpenXR instance creation
 * and graphics requirements retrieval.
 */

#include "../qcommon/q_shared.h"

#define XR_USE_GRAPHICS_API_OPENGL
#ifdef _WIN32
#define XR_USE_PLATFORM_WIN32
#include <windows.h>
#include <GL/gl.h>
#elif defined(__linux__)
#define XR_USE_PLATFORM_XLIB
#include <X11/Xlib.h>
#include <GL/glx.h>
#endif

#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>

#include "vr_gl.h"
#include "vr_gl_types.h"
#include "../vrcommon/vr_backend.h"

// Get the OpenGL graphics extension name for OpenXR instance creation
const char* VR_GL_GetGraphicsExtensionName(void)
{
	return XR_KHR_OPENGL_ENABLE_EXTENSION_NAME;
}

// Get OpenGL graphics requirements from OpenXR
// Must be called after VR_Init() creates the XR instance
XrResult VR_GL_GetGraphicsRequirements(XrInstance instance, XrSystemId systemId,
                                        VR_GL_GraphicsRequirements* requirements)
{
	PFN_xrGetOpenGLGraphicsRequirementsKHR pfnGetOpenGLGraphicsRequirementsKHR = NULL;
	XrResult result = xrGetInstanceProcAddr(
		instance,
		"xrGetOpenGLGraphicsRequirementsKHR",
		(PFN_xrVoidFunction*)&pfnGetOpenGLGraphicsRequirementsKHR);

	if (XR_FAILED(result) || pfnGetOpenGLGraphicsRequirementsKHR == NULL)
	{
		return result;
	}

	requirements->requirements.type = XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_KHR;
	requirements->requirements.next = NULL;

	return pfnGetOpenGLGraphicsRequirementsKHR(instance, systemId, &requirements->requirements);
}

// Print graphics requirements debug info
void VR_GL_PrintGraphicsRequirements(const VR_GL_GraphicsRequirements* requirements)
{
	fprintf(stderr, "[OpenXR] OpenGL version requirements: [%u.%u, %u.%u]\n",
		XR_VERSION_MAJOR(requirements->requirements.minApiVersionSupported),
		XR_VERSION_MINOR(requirements->requirements.minApiVersionSupported),
		XR_VERSION_MAJOR(requirements->requirements.maxApiVersionSupported),
		XR_VERSION_MINOR(requirements->requirements.maxApiVersionSupported));
}

// Initialize OpenGL-specific VR subsystems
void VR_GL_Init(void)
{
	// Currently no additional initialization needed
	// Graphics requirements are fetched separately via VR_GL_GetGraphicsRequirements
}

// Shutdown OpenGL-specific VR subsystems
void VR_GL_Shutdown(void)
{
	// Currently no cleanup needed
}

// ============================================================================
// VR_Graphics interface implementation for OpenGL
// These functions are called from vrcommon code and provide the graphics-specific
// behavior for OpenGL builds.
// ============================================================================

// Cached graphics requirements
static VR_GL_GraphicsRequirements s_glRequirements;
static qboolean s_requirementsFetched = qfalse;

const char* VRGL_GetExtensionName(void)
{
	return VR_GL_GetGraphicsExtensionName();
}

XrResult VRGL_GetRequirements(XrInstance instance, XrSystemId systemId)
{
	XrResult result = VR_GL_GetGraphicsRequirements(instance, systemId, &s_glRequirements);
	if (XR_SUCCEEDED(result)) {
		s_requirementsFetched = qtrue;
	}
	return result;
}

void VRGL_PrintRequirements(void)
{
	if (s_requirementsFetched) {
		VR_GL_PrintGraphicsRequirements(&s_glRequirements);
	}
}

void VRGL_GraphicsInit(XrInstance instance, XrSystemId systemId)
{
	(void)instance;
	(void)systemId;
	VR_GL_Init();
}

void VRGL_GraphicsShutdown(void)
{
	VR_GL_Shutdown();
	s_requirementsFetched = qfalse;
}

void VRGL_InvalidateFunctionPointers(void)
{
	// OpenGL doesn't cache XR function pointers at module level like Vulkan does.
	// VRGL_GetRequirements fetches xrGetOpenGLGraphicsRequirementsKHR
	// locally each time, so there's nothing to invalidate. However, we do need
	// to clear the requirements fetched flag so they're re-fetched after vid_restart.
	s_requirementsFetched = qfalse;
}

static void VRGL_GetColorSwapchainDesc( const VR_SwapchainInfos *sw,
                                        XrSwapchain *handle, int *width, int *height )
{
	*handle = sw->color.swapchain;
	*width  = sw->color.width;
	*height = sw->color.height;
}

const vr_backend_t* VRGL_GetBackend( void )
{
	static const vr_backend_t backend = {
		.GetExtensionName = VRGL_GetExtensionName,
		.GetRequirements = VRGL_GetRequirements,
		.PrintRequirements = VRGL_PrintRequirements,
		.GraphicsInit = VRGL_GraphicsInit,
		.GraphicsShutdown = VRGL_GraphicsShutdown,
		.InvalidateFunctionPointers = VRGL_InvalidateFunctionPointers,
		.CreateSession = VRGL_CreateSession,
		.GetResolution = VRGL_GetResolution,
		.InitRenderer = VRGL_InitRenderer,
		.DestroyRenderer = VRGL_DestroyRenderer,
		.ProcessFrame = VRGL_ProcessFrame,
		.RestoreState = VRGL_RestoreState,
		.SubmitLoadingFrame = VRGL_SubmitLoadingFrame,
		.FinishFrame = VRGL_FinishFrame,
		.GetColorSwapchainDesc = VRGL_GetColorSwapchainDesc,
		.GetStencilBits = VRGL_GetStencilBits,
	};
	return &backend;
}
