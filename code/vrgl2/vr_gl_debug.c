#include "vr_gl_debug.h"

#include <stdio.h>

#include "../renderercommon/tr_common.h"   // qgl function-pointer declarations

//
// OpenGL Debug
//

void APIENTRY VR_GL_DebugLogSink(GLenum source, GLenum type, GLuint id,
	GLenum severity, GLsizei length, const GLchar* message, const void* userParam);

void VR_GL_RegisterDebugLogSinkIfEnabled(void)
{
#if VR_ENABLE_GL_DEBUG
	glEnable(GL_DEBUG_OUTPUT);
	qglDebugMessageCallback(VR_GL_DebugLogSink, 0);
#endif
}

#if VR_ENABLE_GL_DEBUG
void APIENTRY VR_GL_DebugLogSink(GLenum source, GLenum type, GLuint id,
	GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
{
	if (type == GL_DEBUG_TYPE_ERROR || type == GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR || VR_ENABLE_GL_DEBUG_VERBOSE)
	{
		char typeStr[128];
		switch (type)
		{
			case GL_DEBUG_TYPE_ERROR: sprintf(typeStr, "ERROR"); break;
			case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: sprintf(typeStr, "DEPRECATED_BEHAVIOR"); break;
			case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: sprintf(typeStr, "UNDEFINED_BEHAVIOR"); break;
			case GL_DEBUG_TYPE_PORTABILITY: sprintf(typeStr, "PORTABILITY"); break;
			case GL_DEBUG_TYPE_PERFORMANCE: sprintf(typeStr, "PERFORMANCE"); break;
			case GL_DEBUG_TYPE_MARKER: sprintf(typeStr, "MARKER"); break;
			case GL_DEBUG_TYPE_PUSH_GROUP: sprintf(typeStr, "PUSH_GROUP"); break;
			case GL_DEBUG_TYPE_POP_GROUP: sprintf(typeStr, "POP_GROUP"); break;
			default: sprintf(typeStr, "OTHER"); break;
		}

		char severinityStr[128];
		switch (severity)
		{
			case GL_DEBUG_SEVERITY_HIGH: sprintf(severinityStr, "HIGH"); break;
			case GL_DEBUG_SEVERITY_MEDIUM: sprintf(severinityStr, "MEDIUM"); break;
			case GL_DEBUG_SEVERITY_LOW: sprintf(severinityStr, "LOW"); break;
			default: return; sprintf(severinityStr, "VERBOSE"); break;
		}

		fprintf(stderr, "[%s] GL issue - %s: %s\n", severinityStr, typeStr, message);
	}
}
#endif
