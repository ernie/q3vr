/*
 * vr_gl_session.c - OpenGL-specific XR session creation
 *
 * Creates XrSession with OpenGL graphics binding (XR_KHR_opengl_enable).
 * Platform-specific bindings for Win32 and Xlib.
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

#include <string.h>

#ifdef USE_INTERNAL_SDL_HEADERS
#include "SDL_syswm.h"
#else
#include <SDL_syswm.h>
#endif

#include "../vrcommon/vr_macros.h"

// Create XR session with OpenGL graphics binding
XrResult VR_GL_CreateSession(XrInstance instance, XrSystemId systemId, XrSession* session)
{
#if defined(XR_USE_PLATFORM_WIN32)
	// OpenGL Windows binding
	XrGraphicsBindingOpenGLWin32KHR graphicsBinding = {};
	graphicsBinding.type = XR_TYPE_GRAPHICS_BINDING_OPENGL_WIN32_KHR;
	graphicsBinding.next = NULL;
	graphicsBinding.hDC = wglGetCurrentDC();
	graphicsBinding.hGLRC = wglGetCurrentContext();

	CHECK(graphicsBinding.hDC != NULL, "[OpenXR] Failed to obtain current Device Context");
	CHECK(graphicsBinding.hGLRC != NULL, "[OpenXR] Failed to obtain current OpenGL RC");

#elif defined(XR_USE_PLATFORM_XLIB)
	// OpenGL Linux/X11 binding
	extern SDL_Window *SDL_window;
	SDL_SysWMinfo wminfo;
	SDL_VERSION(&wminfo.version);
	SDL_GetWindowWMInfo(SDL_window, &wminfo);

	XrGraphicsBindingOpenGLXlibKHR graphicsBinding;
	memset(&graphicsBinding, 0, sizeof(graphicsBinding));
	graphicsBinding.type = XR_TYPE_GRAPHICS_BINDING_OPENGL_XLIB_KHR;
	graphicsBinding.next = NULL;
	graphicsBinding.xDisplay = wminfo.info.x11.display;
	graphicsBinding.glxContext = glXGetCurrentContext();
	graphicsBinding.glxDrawable = glXGetCurrentDrawable();

#else
#	error "Unsupported platform for OpenGL"
#endif

	XrSessionCreateInfo sessionCreateInfo = {};
	memset(&sessionCreateInfo, 0, sizeof(sessionCreateInfo));
	sessionCreateInfo.type = XR_TYPE_SESSION_CREATE_INFO;
	sessionCreateInfo.next = &graphicsBinding;
	sessionCreateInfo.createFlags = 0;
	sessionCreateInfo.systemId = systemId;

	return xrCreateSession(instance, &sessionCreateInfo, session);
}

// vr_backend_t interface implementation
XrResult VRGL_CreateSession(XrInstance instance, XrSystemId systemId, XrSession* session)
{
	return VR_GL_CreateSession(instance, systemId, session);
}
