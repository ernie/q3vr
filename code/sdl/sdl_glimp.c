/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

#ifdef USE_INTERNAL_SDL_HEADERS
#	include "SDL.h"
#else
#	include <SDL.h>
#endif

#include <vulkan/vulkan.h>
#include <SDL_vulkan.h>

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "../renderercommon/tr_common.h"
#include "../sys/sys_local.h"
#include "../client/client.h"
#include "sdl_icon.h"

#include "../vrcommon/vr_base.h"
#include "../vrcommon/vr_input.h"
#include "../vrcommon/vr_renderer.h"

typedef enum
{
	RSERR_OK,

	RSERR_INVALID_FULLSCREEN,
	RSERR_INVALID_MODE,

	RSERR_UNKNOWN
} rserr_t;

SDL_Window *SDL_window = NULL;
static SDL_GLContext SDL_glContext = NULL;

// The renderer owns glconfig_t (frozen ABI); the client-side glimp fills the
// caller's instance through this pointer, set at the top of GLimp_Init/VKimp_Init.
glconfig_t *glimp_config = NULL;

// Used only by client-side mode selection; renderers never read it.
float displayAspect = 0.0f;

cvar_t *r_allowSoftwareGL; // Don't abort out if a hardware visual can't be obtained
cvar_t *r_allowResize; // make window resizable
cvar_t *r_centerWindow;
cvar_t *r_sdlDriver;
cvar_t *r_preferOpenGLES;

int qglMajorVersion, qglMinorVersion;
int qglesMajorVersion, qglesMinorVersion;

void (APIENTRYP qglActiveTextureARB) (GLenum texture);
void (APIENTRYP qglClientActiveTextureARB) (GLenum texture);
void (APIENTRYP qglMultiTexCoord2fARB) (GLenum target, GLfloat s, GLfloat t);

void (APIENTRYP qglLockArraysEXT) (GLint first, GLsizei count);
void (APIENTRYP qglUnlockArraysEXT) (void);

#define GLE(ret, name, ...) name##proc * qgl##name = NULL;
QGL_1_1_PROCS;
QGL_1_1_FIXED_FUNCTION_PROCS;
QGL_DESKTOP_1_1_PROCS;
QGL_DESKTOP_1_1_FIXED_FUNCTION_PROCS;
QGL_ES_1_1_PROCS;
QGL_ES_1_1_FIXED_FUNCTION_PROCS;
QGL_1_3_PROCS;
QGL_1_5_PROCS;
QGL_2_0_PROCS;
QGL_3_0_PROCS;
QGL_3_1_PROCS;
QGL_3_2_PROCS;
QGL_4_2_PROCS;
QGL_4_3_PROCS;
QGL_4_5_PROCS;
QGL_OVR_multiview_PROCS;
QGL_ARB_occlusion_query_PROCS;
QGL_ARB_framebuffer_object_PROCS;
QGL_ARB_vertex_array_object_PROCS;
QGL_EXT_direct_state_access_PROCS;
#undef GLE

/*
===============
GLimp_Shutdown
===============
*/
void GLimp_Shutdown( void )
{
	IN_Shutdown();

	// [OpenXR] Destroy renderer and current XR session due to loss of GL context,
	// will recreate it on next renderer init.
	VR_Engine* engine = VR_GetEngine();
	VR_DestroySessionInput(engine);
	VR_DestroyRenderer(engine);
	VR_LeaveVR(engine);

	// Reinitialize OpenXR instance and system, otherwise there might be visual
	// glitches/problems with swapchains when new session is (re)created.
	VR_Destroy(engine);
	VR_Init();

	SDL_QuitSubSystem( SDL_INIT_VIDEO );

	glimp_config = NULL;
}

/*
===============
GLimp_Minimize

Minimize the game so that user is back at the desktop
===============
*/
void GLimp_Minimize( void )
{
	SDL_MinimizeWindow( SDL_window );
}


/*
===============
GLimp_LogComment
===============
*/
void GLimp_LogComment( char *comment )
{
}

/*
===============
GLimp_CompareModes
===============
*/
static int GLimp_CompareModes( const void *a, const void *b )
{
	const float ASPECT_EPSILON = 0.001f;
	SDL_Rect *modeA = (SDL_Rect *)a;
	SDL_Rect *modeB = (SDL_Rect *)b;
	float aspectA = (float)modeA->w / (float)modeA->h;
	float aspectB = (float)modeB->w / (float)modeB->h;
	int areaA = modeA->w * modeA->h;
	int areaB = modeB->w * modeB->h;
	float aspectDiffA = fabs( aspectA - displayAspect );
	float aspectDiffB = fabs( aspectB - displayAspect );
	float aspectDiffsDiff = aspectDiffA - aspectDiffB;

	if( aspectDiffsDiff > ASPECT_EPSILON )
		return 1;
	else if( aspectDiffsDiff < -ASPECT_EPSILON )
		return -1;
	else
		return areaA - areaB;
}


/*
===============
GLimp_DetectAvailableModes
===============
*/
static void GLimp_DetectAvailableModes(void)
{
	int i, j;
	char buf[ MAX_STRING_CHARS ] = { 0 };
	int numSDLModes;
	SDL_Rect *modes;
	int numModes = 0;

	SDL_DisplayMode windowMode;
	int display = SDL_GetWindowDisplayIndex( SDL_window );
	if( display < 0 )
	{
		Com_Printf( "Couldn't get window display index, no resolutions detected: %s\n", SDL_GetError() );
		return;
	}
	numSDLModes = SDL_GetNumDisplayModes( display );

	if( SDL_GetWindowDisplayMode( SDL_window, &windowMode ) < 0 || numSDLModes <= 0 )
	{
		Com_Printf( "Couldn't get window display mode, no resolutions detected: %s\n", SDL_GetError() );
		return;
	}

	modes = SDL_calloc( (size_t)numSDLModes, sizeof( SDL_Rect ) );
	if ( !modes )
	{
		Com_Error( ERR_FATAL, "Out of memory" );
	}

	for( i = 0; i < numSDLModes; i++ )
	{
		SDL_DisplayMode mode;

		if( SDL_GetDisplayMode( display, i, &mode ) < 0 )
			continue;

		if( !mode.w || !mode.h )
		{
			Com_Printf( "Display supports any resolution\n" );
			SDL_free( modes );
			return;
		}

		if( windowMode.format != mode.format )
			continue;

		// SDL can give the same resolution with different refresh rates.
		// Only list resolution once.
		for( j = 0; j < numModes; j++ )
		{
			if( mode.w == modes[ j ].w && mode.h == modes[ j ].h )
				break;
		}

		if( j != numModes )
			continue;

		modes[ numModes ].w = mode.w;
		modes[ numModes ].h = mode.h;
		numModes++;
	}

	if( numModes > 1 )
		qsort( modes, numModes, sizeof( SDL_Rect ), GLimp_CompareModes );

	for( i = 0; i < numModes; i++ )
	{
		const char *newModeString = va( "%ux%u ", modes[ i ].w, modes[ i ].h );

		if( strlen( newModeString ) < (int)sizeof( buf ) - strlen( buf ) )
			Q_strcat( buf, sizeof( buf ), newModeString );
		else
			Com_Printf( "Skipping mode %ux%u, buffer too small\n", modes[ i ].w, modes[ i ].h );
	}

	if( *buf )
	{
		buf[ strlen( buf ) - 1 ] = 0;
		Com_Printf( "Available modes: '%s'\n", buf );
		Cvar_Set( "r_availableModes", buf );
	}
	SDL_free( modes );
}

/*
===============
OpenGL ES compatibility
===============
*/
static void APIENTRY GLimp_GLES_ClearDepth( GLclampd depth ) {
	qglClearDepthf( depth );
}

static void APIENTRY GLimp_GLES_DepthRange( GLclampd near_val, GLclampd far_val ) {
	qglDepthRangef( near_val, far_val );
}

static void APIENTRY GLimp_GLES_DrawBuffer( GLenum mode ) {
	// unsupported
}

static void APIENTRY GLimp_GLES_PolygonMode( GLenum face, GLenum mode ) {
	// unsupported
}

/*
===============
GLimp_GetProcAddresses

Get addresses for OpenGL functions.
===============
*/
static qboolean GLimp_GetProcAddresses( qboolean fixedFunction ) {
	qboolean success = qtrue;
	const char *version;

#ifdef __SDL_NOGETPROCADDR__
#define GLE( ret, name, ... ) qgl##name = gl#name;
#else
#define GLE( ret, name, ... ) qgl##name = (name##proc *) SDL_GL_GetProcAddress("gl" #name); \
	if ( qgl##name == NULL ) { \
		Com_Printf( "ERROR: Missing OpenGL function %s\n", "gl" #name ); \
		success = qfalse; \
	}
#endif

	// OpenGL 1.0 and OpenGL ES 1.0
	GLE(const GLubyte *, GetString, GLenum name)

	if ( !qglGetString ) {
		Com_Error( ERR_FATAL, "glGetString is NULL" );
	}

	version = (const char *)qglGetString( GL_VERSION );

	if ( !version ) {
		Com_Error( ERR_FATAL, "GL_VERSION is NULL" );
	}

	if ( Q_stricmpn( "OpenGL ES", version, 9 ) == 0 ) {
		char profile[6]; // ES, ES-CM, or ES-CL
		sscanf( version, "OpenGL %5s %d.%d", profile, &qglesMajorVersion, &qglesMinorVersion );
		// common lite profile (no floating point) is not supported
		if ( Q_stricmp( profile, "ES-CL" ) == 0 ) {
			qglesMajorVersion = 0;
			qglesMinorVersion = 0;
		}
	} else {
		sscanf( version, "%d.%d", &qglMajorVersion, &qglMinorVersion );
	}

	if ( fixedFunction ) {
		if ( QGL_VERSION_ATLEAST( 1, 1 ) ) {
			QGL_1_1_PROCS;
			QGL_1_1_FIXED_FUNCTION_PROCS;
			QGL_DESKTOP_1_1_PROCS;
			QGL_DESKTOP_1_1_FIXED_FUNCTION_PROCS;
		} else if ( qglesMajorVersion == 1 && qglesMinorVersion >= 1 ) {
			// OpenGL ES 1.1 (2.0 is not backward compatible)
			QGL_1_1_PROCS;
			QGL_1_1_FIXED_FUNCTION_PROCS;
			QGL_ES_1_1_PROCS;
			QGL_ES_1_1_FIXED_FUNCTION_PROCS;
			// error so this doesn't segfault due to NULL desktop GL functions being used
			Com_Error( ERR_FATAL, "Unsupported OpenGL Version: %s", version );
		} else {
			Com_Error( ERR_FATAL, "Unsupported OpenGL Version (%s), OpenGL 1.1 is required", version );
		}
	} else {
		if ( QGL_VERSION_ATLEAST( 2, 0 ) ) {
			QGL_1_1_PROCS;
			QGL_DESKTOP_1_1_PROCS;
			QGL_1_3_PROCS;
			QGL_1_5_PROCS;
			QGL_2_0_PROCS;
		} else if ( QGLES_VERSION_ATLEAST( 2, 0 ) ) {
			QGL_1_1_PROCS;
			QGL_ES_1_1_PROCS;
			QGL_1_3_PROCS;
			QGL_1_5_PROCS;
			QGL_2_0_PROCS;

			qglClearDepth = GLimp_GLES_ClearDepth;
			qglDepthRange = GLimp_GLES_DepthRange;
			qglDrawBuffer = GLimp_GLES_DrawBuffer;
			qglPolygonMode = GLimp_GLES_PolygonMode;
		} else {
			Com_Error( ERR_FATAL, "Unsupported OpenGL Version (%s), OpenGL 2.0 is required", version );
		}
	}

	if ( QGL_VERSION_ATLEAST( 3, 0 ) || QGLES_VERSION_ATLEAST( 3, 0 ) ) {
		QGL_3_0_PROCS;
		// Core since 3.0; vrgl2 needs these for XR framebuffers/vertex arrays.
		QGL_ARB_framebuffer_object_PROCS;
		QGL_ARB_vertex_array_object_PROCS;
	}
	if ( QGL_VERSION_ATLEAST( 3, 1 ) ) {
		QGL_3_1_PROCS;
	}
	if ( QGL_VERSION_ATLEAST( 3, 2 ) ) {
		QGL_3_2_PROCS;
	}
  if ( QGL_VERSION_ATLEAST( 4, 2 ) ) {
		QGL_4_2_PROCS;
	}
	if ( QGL_VERSION_ATLEAST( 4, 3 ) ) {
		QGL_4_3_PROCS;
	}
	if ( QGL_VERSION_ATLEAST( 4, 5 ) ) {
		QGL_4_5_PROCS;
	}
	if ( QGL_VERSION_ATLEAST( 4, 5 ) ) {
		QGL_OVR_multiview_PROCS;
	}

#undef GLE

	return success;
}

/*
===============
GLimp_ClearProcAddresses

Clear addresses for OpenGL functions.
===============
*/
static void GLimp_ClearProcAddresses( void ) {
#define GLE( ret, name, ... ) qgl##name = NULL;

	qglMajorVersion = 0;
	qglMinorVersion = 0;
	qglesMajorVersion = 0;
	qglesMinorVersion = 0;

	QGL_1_1_PROCS;
	QGL_1_1_FIXED_FUNCTION_PROCS;
	QGL_DESKTOP_1_1_PROCS;
	QGL_DESKTOP_1_1_FIXED_FUNCTION_PROCS;
	QGL_ES_1_1_PROCS;
	QGL_ES_1_1_FIXED_FUNCTION_PROCS;
	QGL_1_3_PROCS;
	QGL_1_5_PROCS;
	QGL_2_0_PROCS;
	QGL_3_0_PROCS;
	QGL_ARB_occlusion_query_PROCS;
	QGL_ARB_framebuffer_object_PROCS;
	QGL_ARB_vertex_array_object_PROCS;
	QGL_EXT_direct_state_access_PROCS;

	qglActiveTextureARB = NULL;
	qglClientActiveTextureARB = NULL;
	qglMultiTexCoord2fARB = NULL;

	qglLockArraysEXT = NULL;
	qglUnlockArraysEXT = NULL;

#undef GLE
}

/*
===============
GLimp_SetMode
===============
*/
static int GLimp_SetMode(int mode, qboolean fullscreen, qboolean noborder, qboolean fixedFunction)
{
	struct GLimp_ContextType {
		int profileMask;
		int majorVersion;
		int minorVersion;
	} contexts[4];
	int numContexts, type;
	const char *glstring;
	int perChannelColorBits;
	int colorBits, depthBits, stencilBits;
	int samples;
	int i = 0;
	SDL_Surface *icon = NULL;
	Uint32 flags = SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL;
	SDL_DisplayMode desktopMode;
	int display = 0;
	int x = SDL_WINDOWPOS_UNDEFINED, y = SDL_WINDOWPOS_UNDEFINED;

	// Renderer-owned cvars (renderergl2/tr_init.c registration: default+flags matched)
	cvar_t *cvColorbits = Cvar_Get( "r_colorbits", "0", CVAR_ARCHIVE | CVAR_LATCH );
	cvar_t *cvDepthbits = Cvar_Get( "r_depthbits", "0", CVAR_ARCHIVE | CVAR_LATCH );
	cvar_t *cvStencilbits = Cvar_Get( "r_stencilbits", "8", CVAR_ARCHIVE | CVAR_LATCH );
	cvar_t *cvSwapInterval = Cvar_Get( "r_swapInterval", "0", CVAR_ARCHIVE | CVAR_LATCH );

	Com_Printf( "Initializing OpenGL display\n");

	if ( r_allowResize->integer )
		flags |= SDL_WINDOW_RESIZABLE;

#ifdef USE_ICON
	icon = SDL_CreateRGBSurfaceFrom(
			(void *)CLIENT_WINDOW_ICON.pixel_data,
			CLIENT_WINDOW_ICON.width,
			CLIENT_WINDOW_ICON.height,
			CLIENT_WINDOW_ICON.bytes_per_pixel * 8,
			CLIENT_WINDOW_ICON.bytes_per_pixel * CLIENT_WINDOW_ICON.width,
#ifdef Q3_LITTLE_ENDIAN
			0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000
#else
			0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF
#endif
			);
#endif

	// If a window exists, note its display index
	if( SDL_window != NULL )
	{
		display = SDL_GetWindowDisplayIndex( SDL_window );
		if( display < 0 )
		{
			Com_DPrintf( "SDL_GetWindowDisplayIndex() failed: %s\n", SDL_GetError() );
			display = 0;
		}
	}

	if( SDL_GetDesktopDisplayMode( display, &desktopMode ) == 0 )
	{
		displayAspect = (float)desktopMode.w / (float)desktopMode.h;

		Com_Printf( "Display aspect: %.3f\n", displayAspect );
	}
	else
	{
		Com_Memset( &desktopMode, 0, sizeof( SDL_DisplayMode ) );

		Com_Printf( "Cannot determine display aspect, assuming 1.333\n" );
	}

	Com_Printf( "...setting mode %d:", mode );

  VR_Engine* engine = VR_GetEngine();
	VR_GetResolution(engine, &glimp_config->vidWidth, &glimp_config->vidHeight);

	const int desktopWidth = Cvar_VariableIntegerValue("r_customdesktopwidth");
	const int desktopHeight = Cvar_VariableIntegerValue("r_customdesktopheight");
	if (desktopWidth <= 0 || desktopHeight <= 0)
	{
		Cvar_SetValue("r_customdesktopwidth", desktopMode.w);
		Cvar_SetValue("r_customdesktopheight", desktopMode.h);
	} else {
		desktopMode.w = desktopWidth;
		desktopMode.h = desktopHeight;
	}
	engine->window.width = desktopMode.w;
  engine->window.height = desktopMode.h;
#if 0
	if (mode == -2)
	{
		// use desktop video resolution
		if( desktopMode.h > 0 )
		{
			glimp_config->vidWidth = desktopMode.w;
			glimp_config->vidHeight = desktopMode.h;
		}
		else
		{
			glimp_config->vidWidth = 640;
			glimp_config->vidHeight = 480;
			Com_Printf( "Cannot determine display resolution, assuming 640x480\n" );
		}

		glimp_config->windowAspect = (float)glimp_config->vidWidth / (float)glimp_config->vidHeight;
	}
	else if ( !R_GetModeInfo( &glimp_config->vidWidth, &glimp_config->vidHeight, &glimp_config->windowAspect, mode ) )
	{
		Com_Printf( " invalid mode\n" );
		return RSERR_INVALID_MODE;
	}
#endif
	Com_Printf( " %d %d\n", glimp_config->vidWidth, glimp_config->vidHeight);

	// Center window
	if( r_centerWindow->integer && !fullscreen )
	{
		x = ( desktopMode.w / 2 ) - ( glimp_config->vidWidth / 2 );
		y = ( desktopMode.h / 2 ) - ( glimp_config->vidHeight / 2 );
	}

	// Destroy existing state if it exists
	if( SDL_glContext != NULL )
	{
		GLimp_ClearProcAddresses();
		SDL_GL_DeleteContext( SDL_glContext );
		SDL_glContext = NULL;
	}

	if( SDL_window != NULL )
	{
		SDL_GetWindowPosition( SDL_window, &x, &y );
		Com_DPrintf( "Existing window at %dx%d before being destroyed\n", x, y );
		SDL_DestroyWindow( SDL_window );
		SDL_window = NULL;
	}

	if( fullscreen )
	{
		flags |= SDL_WINDOW_FULLSCREEN;
		glimp_config->isFullscreen = qtrue;
	}
	else
	{
		if( noborder )
			flags |= SDL_WINDOW_BORDERLESS;

		glimp_config->isFullscreen = qfalse;
	}

	colorBits = cvColorbits->value;
	if ((!colorBits) || (colorBits >= 32))
		colorBits = 24;

	if (!cvDepthbits->value)
		depthBits = 24;
	else
		depthBits = cvDepthbits->value;

	stencilBits = cvStencilbits->value;
	samples = 0; // r_ext_multisample is not relevant for renderergl2
	numContexts = 0;

#if 0
	if ( !fixedFunction ) {
		int profileMask;
		qboolean preferOpenGLES;

		SDL_GL_ResetAttributes();
		SDL_GL_GetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, &profileMask );

		preferOpenGLES = ( r_preferOpenGLES->integer == 1 ||
		                 ( r_preferOpenGLES->integer == -1 && profileMask == SDL_GL_CONTEXT_PROFILE_ES ) );

		if ( preferOpenGLES ) {
#ifdef __EMSCRIPTEN__
			// WebGL 2.0 isn't fully backward compatible so you have to ask for it specifically
			contexts[numContexts].profileMask = SDL_GL_CONTEXT_PROFILE_ES;
			contexts[numContexts].majorVersion = 3;
			contexts[numContexts].minorVersion = 0;
			numContexts++;
#endif

			contexts[numContexts].profileMask = SDL_GL_CONTEXT_PROFILE_ES;
			contexts[numContexts].majorVersion = 2;
			contexts[numContexts].minorVersion = 0;
			numContexts++;
		}

		contexts[numContexts].profileMask = SDL_GL_CONTEXT_PROFILE_CORE;
		contexts[numContexts].majorVersion = 3;
		contexts[numContexts].minorVersion = 2;
		numContexts++;

		contexts[numContexts].profileMask = 0;
		contexts[numContexts].majorVersion = 2;
		contexts[numContexts].minorVersion = 0;
		numContexts++;

		if ( !preferOpenGLES ) {
#ifdef __EMSCRIPTEN__
			contexts[numContexts].profileMask = SDL_GL_CONTEXT_PROFILE_ES;
			contexts[numContexts].majorVersion = 3;
			contexts[numContexts].minorVersion = 0;
			numContexts++;
#endif

			contexts[numContexts].profileMask = SDL_GL_CONTEXT_PROFILE_ES;
			contexts[numContexts].majorVersion = 2;
			contexts[numContexts].minorVersion = 0;
			numContexts++;
		}
	} else {
		contexts[numContexts].profileMask = 0;
		contexts[numContexts].majorVersion = 1;
		contexts[numContexts].minorVersion = 1;
		numContexts++;
	}
#endif
  contexts[0].majorVersion = 4;
  contexts[0].minorVersion = 6;
  contexts[0].profileMask = SDL_GL_CONTEXT_PROFILE_CORE;
  numContexts = 1;

	for (i = 0; i < 16; i++)
	{
		int testColorBits, testDepthBits, testStencilBits;
		int realColorBits[3];

		// 0 - default
		// 1 - minus colorBits
		// 2 - minus depthBits
		// 3 - minus stencil
		if ((i % 4) == 0 && i)
		{
			// one pass, reduce
			switch (i / 4)
			{
				case 2 :
					if (colorBits == 24)
						colorBits = 16;
					break;
				case 1 :
					if (depthBits == 24)
						depthBits = 16;
					else if (depthBits == 16)
						depthBits = 8;
				case 3 :
					if (stencilBits == 24)
						stencilBits = 16;
					else if (stencilBits == 16)
						stencilBits = 8;
			}
		}

		testColorBits = colorBits;
		testDepthBits = depthBits;
		testStencilBits = stencilBits;

		if ((i % 4) == 3)
		{ // reduce colorBits
			if (testColorBits == 24)
				testColorBits = 16;
		}

		if ((i % 4) == 2)
		{ // reduce depthBits
			if (testDepthBits == 24)
				testDepthBits = 16;
			else if (testDepthBits == 16)
				testDepthBits = 8;
		}

		if ((i % 4) == 1)
		{ // reduce stencilBits
			if (testStencilBits == 24)
				testStencilBits = 16;
			else if (testStencilBits == 16)
				testStencilBits = 8;
			else
				testStencilBits = 0;
		}

		if (testColorBits == 24)
			perChannelColorBits = 8;
		else
			perChannelColorBits = 4;

#ifdef __sgi /* Fix for SGIs grabbing too many bits of color */
		if (perChannelColorBits == 4)
			perChannelColorBits = 0; /* Use minimum size for 16-bit color */

		/* Need alpha or else SGIs choose 36+ bit RGB mode */
		SDL_GL_SetAttribute( SDL_GL_ALPHA_SIZE, 1);
#endif

		SDL_GL_SetAttribute( SDL_GL_RED_SIZE, perChannelColorBits );
		SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, perChannelColorBits );
		SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, perChannelColorBits );
		SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, testDepthBits );
		SDL_GL_SetAttribute( SDL_GL_STENCIL_SIZE, testStencilBits );

		SDL_GL_SetAttribute( SDL_GL_MULTISAMPLEBUFFERS, samples ? 1 : 0 );
		SDL_GL_SetAttribute( SDL_GL_MULTISAMPLESAMPLES, samples );

#if 0
		if(r_stereoEnabled->integer)
		{
			glimp_config->stereoEnabled = qtrue;
			SDL_GL_SetAttribute(SDL_GL_STEREO, 1);
		}
		else
		{
			glimp_config->stereoEnabled = qfalse;
			SDL_GL_SetAttribute(SDL_GL_STEREO, 0);
		}
		
#endif
		SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 0 );

#if 0 // if multisampling is enabled on X11, this causes create window to fail.
		// If not allowing software GL, demand accelerated
		if( !r_allowSoftwareGL->integer )
			SDL_GL_SetAttribute( SDL_GL_ACCELERATED_VISUAL, 1 );
#endif

		if( ( SDL_window = SDL_CreateWindow( CLIENT_WINDOW_TITLE, x, y,
				engine->window.width, engine->window.height, flags ) ) == NULL )
		{
			Com_DPrintf( "SDL_CreateWindow failed: %s\n", SDL_GetError( ) );
			continue;
		}

#if 0
		if( fullscreen )
		{
			SDL_DisplayMode desiredMode;

			switch( testColorBits )
			{
				case 16: desiredMode.format = SDL_PIXELFORMAT_RGB565; break;
				case 24: desiredMode.format = SDL_PIXELFORMAT_RGB24;  break;
				default: Com_DPrintf( "testColorBits is %d, can't fullscreen\n", testColorBits ); continue;
			}

			desiredMode.w = glimp_config->vidWidth;
			desiredMode.h = glimp_config->vidHeight;
			desiredMode.refresh_rate = glimp_config->displayFrequency = Cvar_VariableIntegerValue( "r_displayRefresh" );
			desiredMode.driverdata = NULL;

			if( SDL_SetWindowDisplayMode( SDL_window, &desiredMode ) < 0 )
			{
				Com_DPrintf( "SDL_SetWindowDisplayMode failed: %s\n", SDL_GetError( ) );
				continue;
			}
		}

		SDL_SetWindowIcon( SDL_window, icon );
#endif

		for ( type = 0; type < numContexts; type++ ) {
			char contextName[32];

			switch ( contexts[type].profileMask ) {
				default:
				case 0:
					Com_sprintf( contextName, sizeof( contextName ), "OpenGL %d.%d",
					             contexts[type].majorVersion, contexts[type].minorVersion );
					break;
				case SDL_GL_CONTEXT_PROFILE_CORE:
					Com_sprintf( contextName, sizeof( contextName ), "OpenGL %d.%d Core",
					             contexts[type].majorVersion, contexts[type].minorVersion );
					break;
				case SDL_GL_CONTEXT_PROFILE_ES:
					Com_sprintf( contextName, sizeof( contextName ), "OpenGL ES %d.%d",
					             contexts[type].majorVersion, contexts[type].minorVersion );
					break;
			}

			SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, contexts[type].profileMask );
			SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, contexts[type].majorVersion );
			SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, contexts[type].minorVersion );

			SDL_glContext = SDL_GL_CreateContext( SDL_window );
			if ( !SDL_glContext )
			{
				Com_Printf( "SDL_GL_CreateContext() for %s context failed: %s\n", contextName, SDL_GetError() );
				continue;
			}

			if ( !GLimp_GetProcAddresses( fixedFunction ) )
			{
				Com_Printf( "GLimp_GetProcAddresses() for %s context failed\n", contextName );
				GLimp_ClearProcAddresses();
				SDL_GL_DeleteContext( SDL_glContext );
				SDL_glContext = NULL;
				continue;
			}

			if ( contexts[type].profileMask == SDL_GL_CONTEXT_PROFILE_CORE ) {
				const char *renderer;

				renderer = (const char *)qglGetString( GL_RENDERER );

				if ( !renderer || strstr( renderer, "Software Renderer" ) || strstr( renderer, "Software Rasterizer" ) )
				{
					Com_Printf( "GL_RENDERER is %s, rejecting %s context\n", renderer, contextName );

					GLimp_ClearProcAddresses();
					SDL_GL_DeleteContext( SDL_glContext );
					SDL_glContext = NULL;
					continue;
				}
			}

			Com_Printf( "GL_RENDERER with profile %d.%d%s\n", 
					contexts[type].majorVersion, 
					contexts[type].minorVersion, 
					(contexts[type].profileMask == SDL_GL_CONTEXT_PROFILE_CORE ? " (core)" : "")
			);
			break;
		}

		if ( !SDL_glContext ) {
			SDL_DestroyWindow( SDL_window );
			SDL_window = NULL;
			continue;
		}

		qglClearColor( 0, 0, 0, 1 );
		qglClear( GL_COLOR_BUFFER_BIT );
		SDL_GL_SwapWindow( SDL_window );

		if( SDL_GL_SetSwapInterval( cvSwapInterval->integer ) == -1 )
		{
			Com_DPrintf( "SDL_GL_SetSwapInterval failed: %s\n", SDL_GetError( ) );
		}

		SDL_GL_GetAttribute( SDL_GL_RED_SIZE, &realColorBits[0] );
		SDL_GL_GetAttribute( SDL_GL_GREEN_SIZE, &realColorBits[1] );
		SDL_GL_GetAttribute( SDL_GL_BLUE_SIZE, &realColorBits[2] );
		SDL_GL_GetAttribute( SDL_GL_DEPTH_SIZE, &glimp_config->depthBits );
		SDL_GL_GetAttribute( SDL_GL_STENCIL_SIZE, &glimp_config->stencilBits );

		glimp_config->colorBits = realColorBits[0] + realColorBits[1] + realColorBits[2];

		Com_Printf( "Using %d color bits, %d depth, %d stencil display.\n",
				glimp_config->colorBits, glimp_config->depthBits, glimp_config->stencilBits );
		break;
	}

	SDL_FreeSurface( icon );

	if( !SDL_window )
	{
		Com_Printf( "Couldn't get a visual\n" );
		return RSERR_INVALID_MODE;
	}

	GLimp_DetectAvailableModes();

	// Hide window if desktop mirroring is disabled
	if (Cvar_VariableIntegerValue("vr_desktopMode") == 0)
	{
		SDL_HideWindow(SDL_window);
	}

	glstring = (char *) qglGetString (GL_RENDERER);
	Com_Printf( "GL_RENDERER: %s\n", glstring );

	return RSERR_OK;
}

/*
===============
GLimp_StartDriverAndSetMode
===============
*/
static qboolean GLimp_StartDriverAndSetMode(int mode, qboolean fullscreen, qboolean noborder, qboolean gl3Core)
{
	rserr_t err;
	// Renderer-owned cvar (renderergl2/tr_init.c registration: default+flags matched)
	cvar_t *cvFullscreen = Cvar_Get( "r_fullscreen", "1", CVAR_ARCHIVE );

	if (!SDL_WasInit(SDL_INIT_VIDEO))
	{
		const char *driverName;

		if (SDL_Init(SDL_INIT_VIDEO) != 0)
		{
			Com_Printf( "SDL_Init( SDL_INIT_VIDEO ) FAILED (%s)\n", SDL_GetError());
			return qfalse;
		}

		driverName = SDL_GetCurrentVideoDriver( );
		Com_Printf( "SDL using driver \"%s\"\n", driverName );
		Cvar_Set( "r_sdlDriver", driverName );
	}

	if (fullscreen && Cvar_VariableIntegerValue( "in_nograb" ) )
	{
		Com_Printf( "Fullscreen not allowed with in_nograb 1\n");
		Cvar_Set( "r_fullscreen", "0" );
		cvFullscreen->modified = qfalse;
		fullscreen = qfalse;
	}
	
	err = GLimp_SetMode(mode, fullscreen, noborder, gl3Core);

	switch ( err )
	{
		case RSERR_INVALID_FULLSCREEN:
			Com_Printf( "...WARNING: fullscreen unavailable in this mode\n" );
			return qfalse;
		case RSERR_INVALID_MODE:
			Com_Printf( "...WARNING: could not set the given mode (%d)\n", mode );
			return qfalse;
		default:
			break;
	}

	return qtrue;
}


/*
===============
GLimp_InitExtensions
===============
*/
static void GLimp_InitExtensions( qboolean fixedFunction )
{
	// Renderer-owned cvars (renderergl2/tr_init.c registration: default+flags matched)
	cvar_t *cvAllowExtensions = Cvar_Get( "r_allowExtensions", "1", CVAR_ARCHIVE | CVAR_LATCH );
	cvar_t *cvExtCompressedTextures = Cvar_Get( "r_ext_compressed_textures", "0", CVAR_ARCHIVE | CVAR_LATCH );
	cvar_t *cvExtMultitexture = Cvar_Get( "r_ext_multitexture", "1", CVAR_ARCHIVE | CVAR_LATCH );
	cvar_t *cvExtCompiledVertexArray = Cvar_Get( "r_ext_compiled_vertex_array", "1", CVAR_ARCHIVE | CVAR_LATCH );
	cvar_t *cvExtTextureEnvAdd = Cvar_Get( "r_ext_texture_env_add", "1", CVAR_ARCHIVE | CVAR_LATCH );

	if ( !cvAllowExtensions->integer )
	{
		Com_Printf( "* IGNORING OPENGL EXTENSIONS *\n" );
		return;
	}

	Com_Printf( "Initializing OpenGL extensions\n" );

	glimp_config->textureCompression = TC_NONE;

	// GL_EXT_texture_compression_s3tc
	if ( ( QGLES_VERSION_ATLEAST( 2, 0 ) || SDL_GL_ExtensionSupported( "GL_ARB_texture_compression" ) ) &&
	     SDL_GL_ExtensionSupported( "GL_EXT_texture_compression_s3tc" ) )
	{
		if ( cvExtCompressedTextures->value )
		{
			glimp_config->textureCompression = TC_S3TC_ARB;
			Com_Printf( "...using GL_EXT_texture_compression_s3tc\n" );
		}
		else
		{
			Com_Printf( "...ignoring GL_EXT_texture_compression_s3tc\n" );
		}
	}
	else
	{
		Com_Printf( "...GL_EXT_texture_compression_s3tc not found\n" );
	}

	// GL_S3_s3tc ... legacy extension before GL_EXT_texture_compression_s3tc.
	if (glimp_config->textureCompression == TC_NONE)
	{
		if ( SDL_GL_ExtensionSupported( "GL_S3_s3tc" ) )
		{
			if ( cvExtCompressedTextures->value )
			{
				glimp_config->textureCompression = TC_S3TC;
				Com_Printf( "...using GL_S3_s3tc\n" );
			}
			else
			{
				Com_Printf( "...ignoring GL_S3_s3tc\n" );
			}
		}
		else
		{
			Com_Printf( "...GL_S3_s3tc not found\n" );
		}
	}

	// OpenGL 1 fixed function pipeline
	if ( fixedFunction )
	{
		// GL_EXT_texture_env_add
		glimp_config->textureEnvAddAvailable = qfalse;
		if ( SDL_GL_ExtensionSupported( "GL_EXT_texture_env_add" ) )
		{
			if ( cvExtTextureEnvAdd->integer )
			{
				glimp_config->textureEnvAddAvailable = qtrue;
				Com_Printf( "...using GL_EXT_texture_env_add\n" );
			}
			else
			{
				glimp_config->textureEnvAddAvailable = qfalse;
				Com_Printf( "...ignoring GL_EXT_texture_env_add\n" );
			}
		}
		else
		{
			Com_Printf( "...GL_EXT_texture_env_add not found\n" );
		}

		// GL_ARB_multitexture
		qglMultiTexCoord2fARB = NULL;
		qglActiveTextureARB = NULL;
		qglClientActiveTextureARB = NULL;
		if ( SDL_GL_ExtensionSupported( "GL_ARB_multitexture" ) )
		{
			if ( cvExtMultitexture->value )
			{
				qglMultiTexCoord2fARB = SDL_GL_GetProcAddress( "glMultiTexCoord2fARB" );
				qglActiveTextureARB = SDL_GL_GetProcAddress( "glActiveTextureARB" );
				qglClientActiveTextureARB = SDL_GL_GetProcAddress( "glClientActiveTextureARB" );

				if ( qglActiveTextureARB )
				{
					GLint glint = 0;
					qglGetIntegerv( GL_MAX_TEXTURE_UNITS_ARB, &glint );
					glimp_config->numTextureUnits = (int) glint;
					if ( glimp_config->numTextureUnits > 1 )
					{
						Com_Printf( "...using GL_ARB_multitexture\n" );
					}
					else
					{
						qglMultiTexCoord2fARB = NULL;
						qglActiveTextureARB = NULL;
						qglClientActiveTextureARB = NULL;
						Com_Printf( "...not using GL_ARB_multitexture, < 2 texture units\n" );
					}
				}
			}
			else
			{
				Com_Printf( "...ignoring GL_ARB_multitexture\n" );
			}
		}
		else
		{
			Com_Printf( "...GL_ARB_multitexture not found\n" );
		}

		// GL_EXT_compiled_vertex_array
		if ( SDL_GL_ExtensionSupported( "GL_EXT_compiled_vertex_array" ) )
		{
			if ( cvExtCompiledVertexArray->value )
			{
				Com_Printf( "...using GL_EXT_compiled_vertex_array\n" );
				qglLockArraysEXT = ( void ( APIENTRY * )( GLint, GLint ) ) SDL_GL_GetProcAddress( "glLockArraysEXT" );
				qglUnlockArraysEXT = ( void ( APIENTRY * )( void ) ) SDL_GL_GetProcAddress( "glUnlockArraysEXT" );
				if (!qglLockArraysEXT || !qglUnlockArraysEXT)
				{
					Com_Error (ERR_FATAL, "bad getprocaddress");
				}
			}
			else
			{
				Com_Printf( "...ignoring GL_EXT_compiled_vertex_array\n" );
			}
		}
		else
		{
			Com_Printf( "...GL_EXT_compiled_vertex_array not found\n" );
		}
	}

	// Anisotropic/edge-clamp probing moved to the GL renderer DLL
	// (renderergl2/tr_extensions.c); it writes renderer-owned globals.

	if ( SDL_GL_ExtensionSupported( "GL_OVR_multiview2" ) )
	{
		qglFramebufferTextureMultiviewOVR = SDL_GL_GetProcAddress( "glFramebufferTextureMultiviewOVR" );
	}
	else
	{
		Com_Error( ERR_FATAL, "Failed to initialze GL_OVR_multiview2 OpenGL extension\n" );
	}
}

#define R_MODE_FALLBACK 3 // 640 * 480

/*
===============
GLimp_Init

This routine is responsible for initializing the OS specific portions
of OpenGL
===============
*/
void GLimp_Init( glconfig_t *config, qboolean fixedFunction )
{
	glimp_config = config;

	Com_DPrintf( "Glimp_Init( )\n" );

	r_allowSoftwareGL = Cvar_Get( "r_allowSoftwareGL", "0", CVAR_LATCH );
	r_sdlDriver = Cvar_Get( "r_sdlDriver", "", CVAR_ROM );
	r_allowResize = Cvar_Get( "r_allowResize", "0", CVAR_ARCHIVE | CVAR_LATCH );
	r_centerWindow = Cvar_Get( "r_centerWindow", "0", CVAR_ARCHIVE | CVAR_LATCH );
	r_preferOpenGLES = Cvar_Get( "r_preferOpenGLES", "-1", CVAR_ARCHIVE | CVAR_LATCH );

	// Renderer-owned cvars (renderergl2/tr_init.c registration: default+flags matched)
	cvar_t *cvMode = Cvar_Get( "r_mode", "-2", CVAR_ARCHIVE | CVAR_LATCH );
	cvar_t *cvFullscreen = Cvar_Get( "r_fullscreen", "1", CVAR_ARCHIVE );
	cvar_t *cvNoborder = Cvar_Get( "r_noborder", "0", CVAR_ARCHIVE | CVAR_LATCH );
	cvar_t *cvIgnorehwgamma = Cvar_Get( "r_ignorehwgamma", "0", CVAR_ARCHIVE | CVAR_LATCH );

	if( Cvar_VariableIntegerValue( "com_abnormalExit" ) )
	{
		Cvar_Set( "r_mode", va( "%d", R_MODE_FALLBACK ) );
		Cvar_Set( "r_fullscreen", "0" );
		Cvar_Set( "r_centerWindow", "0" );
		Cvar_Set( "com_abnormalExit", "0" );
	}

	Sys_GLimpInit( );

	// Create the window and set up the context
	if(GLimp_StartDriverAndSetMode(cvMode->integer, cvFullscreen->integer, cvNoborder->integer, fixedFunction))
		goto success;

	// Try again, this time in a platform specific "safe mode"
	Sys_GLimpSafeInit( );

	if(GLimp_StartDriverAndSetMode(cvMode->integer, cvFullscreen->integer, qfalse, fixedFunction))
		goto success;

	// Finally, try the default screen resolution
	if( cvMode->integer != R_MODE_FALLBACK )
	{
		Com_Printf( "Setting r_mode %d failed, falling back on r_mode %d\n",
				cvMode->integer, R_MODE_FALLBACK );

		if(GLimp_StartDriverAndSetMode(R_MODE_FALLBACK, qfalse, qfalse, fixedFunction))
			goto success;
	}

	// Nothing worked, give up
	Com_Error( ERR_FATAL, "GLimp_Init() - could not load OpenGL subsystem" );

success:
	// These values force the UI to disable driver selection
	glimp_config->driverType = GLDRV_ICD;
	glimp_config->hardwareType = GLHW_GENERIC;

	// Only using SDL_SetWindowBrightness to determine if hardware gamma is supported
	glimp_config->deviceSupportsGamma = !cvIgnorehwgamma->integer &&
		SDL_SetWindowBrightness( SDL_window, 1.0f ) >= 0;

	// get our config strings
	Q_strncpyz( glimp_config->vendor_string, (char *) qglGetString (GL_VENDOR), sizeof( glimp_config->vendor_string ) );
	Q_strncpyz( glimp_config->renderer_string, (char *) qglGetString (GL_RENDERER), sizeof( glimp_config->renderer_string ) );
	if (*glimp_config->renderer_string && glimp_config->renderer_string[strlen(glimp_config->renderer_string) - 1] == '\n')
		glimp_config->renderer_string[strlen(glimp_config->renderer_string) - 1] = 0;
	Q_strncpyz( glimp_config->version_string, (char *) qglGetString (GL_VERSION), sizeof( glimp_config->version_string ) );

	// manually create extension list if using OpenGL 3
	if ( qglGetStringi )
	{
		int i, numExtensions, extensionLength, listLength;
		const char *extension;

		qglGetIntegerv( GL_NUM_EXTENSIONS, &numExtensions );
		listLength = 0;

		for ( i = 0; i < numExtensions; i++ )
		{
			extension = (char *) qglGetStringi( GL_EXTENSIONS, i );
			extensionLength = strlen( extension );

			if ( ( listLength + extensionLength + 1 ) >= sizeof( glimp_config->extensions_string ) )
				break;

			if ( i > 0 ) {
				Q_strcat( glimp_config->extensions_string, sizeof( glimp_config->extensions_string ), " " );
				listLength++;
			}

			Q_strcat( glimp_config->extensions_string, sizeof( glimp_config->extensions_string ), extension );
			listLength += extensionLength;
		}
	}
	else
	{
		Q_strncpyz( glimp_config->extensions_string, (char *) qglGetString (GL_EXTENSIONS), sizeof( glimp_config->extensions_string ) );
	}

	// initialize extensions
	GLimp_InitExtensions( fixedFunction );

	Cvar_Get( "r_availableModes", "", CVAR_ROM );

	// This depends on SDL_INIT_VIDEO, hence having it here
	IN_Init( SDL_window );
}

// [OpenXR] (Re)create session and renderer if needed
void GLimp_InitVR(void)
{
	VR_Engine* engine = VR_GetEngine();
	if (engine->appState.Session == XR_NULL_HANDLE) {
		VR_EnterVR(engine);
		VR_InitRenderer(engine);
    VR_InitSessionInput(engine);
		VR_Renderer_RestoreState(engine);
	}
}


/*
===============
GLimp_EndFrame

Responsible for doing a swapbuffers
===============
*/
void GLimp_EndFrame( void )
{
	// Renderer-owned cvars; cached after first lookup (runs once per frame).
	static cvar_t *cvDrawBuffer = NULL;
	static cvar_t *cvFullscreen = NULL;

	if ( !cvDrawBuffer )
		cvDrawBuffer = Cvar_Get( "r_drawBuffer", "GL_BACK", CVAR_CHEAT );
	if ( !cvFullscreen )
		cvFullscreen = Cvar_Get( "r_fullscreen", "1", 0 );

	// don't flip if drawing to front buffer
	if ( Q_stricmp( cvDrawBuffer->string, "GL_FRONT" ) != 0 )
	{
		SDL_GL_SwapWindow( SDL_window );
	}

	if( cvFullscreen->modified )
	{
		int         fullscreen;
		qboolean    needToToggle;
		qboolean    sdlToggled = qfalse;

		// Find out the current state
		fullscreen = !!( SDL_GetWindowFlags( SDL_window ) & SDL_WINDOW_FULLSCREEN );

		if( cvFullscreen->integer && Cvar_VariableIntegerValue( "in_nograb" ) )
		{
			Com_Printf( "Fullscreen not allowed with in_nograb 1\n");
			Cvar_Set( "r_fullscreen", "0" );
			cvFullscreen->modified = qfalse;
		}

		// Is the state we want different from the current state?
		needToToggle = !!cvFullscreen->integer != fullscreen;

		if( needToToggle )
		{
			sdlToggled = SDL_SetWindowFullscreen( SDL_window, cvFullscreen->integer ) >= 0;

			// SDL_WM_ToggleFullScreen didn't work, so do it the slow way
			if( !sdlToggled )
				Cbuf_ExecuteText(EXEC_APPEND, "vid_restart\n");

			IN_Restart( );
		}

		cvFullscreen->modified = qfalse;
	}
}


/*
===============
VKimp_SetMode

Creates a Vulkan-compatible SDL window (no OpenGL context).
For Q3VR: Vulkan device is created by VR layer, we just need the window for desktop mirror.
===============
*/
static rserr_t VKimp_SetMode(int mode, qboolean fullscreen, qboolean noborder)
{
	int colorBits, depthBits, stencilBits;
	int samples;
	int x = SDL_WINDOWPOS_UNDEFINED, y = SDL_WINDOWPOS_UNDEFINED;
	Uint32 flags = SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN;
	SDL_DisplayMode desktopMode;
	int display = 0;
	SDL_Surface *icon = NULL;
	// Renderer-owned cvars (renderervk/tr_init.c registration: default+flags matched)
	cvar_t *cvColorbits = Cvar_Get( "r_colorbits", "0", CVAR_ARCHIVE_ND | CVAR_LATCH );
	cvar_t *cvDepthbits = Cvar_Get( "r_depthbits", "0", CVAR_ARCHIVE_ND | CVAR_LATCH );
	cvar_t *cvStencilbits = Cvar_Get( "r_stencilbits", "8", CVAR_ARCHIVE_ND | CVAR_LATCH );

	Com_Printf( "Initializing Vulkan display\n");

	if ( r_allowResize->integer )
		flags |= SDL_WINDOW_RESIZABLE;

#ifdef USE_ICON
	icon = SDL_CreateRGBSurfaceFrom(
			(void *)CLIENT_WINDOW_ICON.pixel_data,
			CLIENT_WINDOW_ICON.width,
			CLIENT_WINDOW_ICON.height,
			CLIENT_WINDOW_ICON.bytes_per_pixel * 8,
			CLIENT_WINDOW_ICON.bytes_per_pixel * CLIENT_WINDOW_ICON.width,
#ifdef Q3_LITTLE_ENDIAN
			0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000
#else
			0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF
#endif
			);
#endif

	if( SDL_window != NULL )
	{
		display = SDL_GetWindowDisplayIndex( SDL_window );
		if( display < 0 )
		{
			Com_DPrintf( "SDL_GetWindowDisplayIndex() failed: %s\n", SDL_GetError() );
			display = 0;
		}
	}

	if( SDL_GetDesktopDisplayMode( display, &desktopMode ) == 0 )
	{
		displayAspect = (float)desktopMode.w / (float)desktopMode.h;
		Com_Printf( "Display aspect: %.3f\n", displayAspect );
	}
	else
	{
		Com_Memset( &desktopMode, 0, sizeof( SDL_DisplayMode ) );
		Com_Printf( "Cannot determine display aspect, assuming 1.333\n" );
	}

	Com_Printf( "...setting mode %d:", mode );

	VR_Engine* engine = VR_GetEngine();
	VR_GetResolution(engine, &glimp_config->vidWidth, &glimp_config->vidHeight);
	glimp_config->windowAspect = (float)glimp_config->vidWidth / (float)glimp_config->vidHeight;

	int windowWidth, windowHeight;

	const int desktopWidth = Cvar_VariableIntegerValue("r_customdesktopwidth");
	const int desktopHeight = Cvar_VariableIntegerValue("r_customdesktopheight");
	if ( desktopWidth <= 0 || desktopHeight <= 0 )
	{
		Cvar_SetValue("r_customdesktopwidth", desktopMode.w);
		Cvar_SetValue("r_customdesktopheight", desktopMode.h);
	}
	else
	{
		desktopMode.w = desktopWidth;
		desktopMode.h = desktopHeight;
	}
	windowWidth = desktopMode.w;
	windowHeight = desktopMode.h;

	Com_Printf( " %d %d\n", glimp_config->vidWidth, glimp_config->vidHeight);

	if( SDL_glContext != NULL )
	{
		SDL_GL_DeleteContext( SDL_glContext );
		SDL_glContext = NULL;
	}

	if( SDL_window != NULL )
	{
		SDL_GetWindowPosition( SDL_window, &x, &y );
		Com_DPrintf( "Existing window at %dx%d before being destroyed\n", x, y );
		SDL_DestroyWindow( SDL_window );
		SDL_window = NULL;
	}

	if( fullscreen )
	{
		flags |= SDL_WINDOW_FULLSCREEN;
		glimp_config->isFullscreen = qtrue;
	}
	else
	{
		if( noborder )
			flags |= SDL_WINDOW_BORDERLESS;

		glimp_config->isFullscreen = qfalse;
	}

	colorBits = cvColorbits->value;
	if (colorBits == 0 || colorBits > 32)
		colorBits = 32;

	if (!cvDepthbits->value)
		depthBits = 24;
	else
		depthBits = cvDepthbits->value;

	stencilBits = cvStencilbits->value;
	samples = 0;

	if( r_centerWindow->integer && !fullscreen )
	{
		x = ( desktopMode.w / 2 ) - ( windowWidth / 2 );
		y = ( desktopMode.h / 2 ) - ( windowHeight / 2 );
	}

	SDL_window = SDL_CreateWindow(CLIENT_WINDOW_TITLE, x, y,
			windowWidth, windowHeight, flags);

	if (!SDL_window)
	{
		Com_Printf( "Couldn't create Vulkan window: %s\n", SDL_GetError() );
		SDL_FreeSurface( icon );
		return RSERR_INVALID_MODE;
	}

	SDL_SetWindowIcon( SDL_window, icon );
	SDL_FreeSurface( icon );

	glimp_config->colorBits = colorBits;
	glimp_config->depthBits = depthBits;
	glimp_config->stencilBits = stencilBits;

	GLimp_DetectAvailableModes();

	engine->window.width = windowWidth;
	engine->window.height = windowHeight;

	if (Cvar_VariableIntegerValue("vr_desktopMode") == 0)
	{
		SDL_HideWindow(SDL_window);
	}

	Com_Printf( "Created Vulkan window %dx%d (XR render: %dx%d)\n",
		windowWidth, windowHeight, glimp_config->vidWidth, glimp_config->vidHeight);

	return RSERR_OK;
}


/*
===============
VKimp_Init

Initialize Vulkan for Q3VR.
This creates the SDL window for desktop mirror.
Vulkan instance/device is already created by the VR layer.
===============
*/
void VKimp_Init(glconfig_t *config)
{
	glimp_config = config;

	Com_DPrintf( "VKimp_Init()\n" );

	r_allowSoftwareGL = Cvar_Get( "r_allowSoftwareGL", "0", CVAR_LATCH );
	r_sdlDriver = Cvar_Get( "r_sdlDriver", "", CVAR_ROM );
	r_allowResize = Cvar_Get( "r_allowResize", "0", CVAR_ARCHIVE | CVAR_LATCH );
	r_centerWindow = Cvar_Get( "r_centerWindow", "0", CVAR_ARCHIVE | CVAR_LATCH );

	// Renderer-owned cvars (renderervk/tr_init.c registration: default+flags matched)
	cvar_t *cvMode = Cvar_Get( "r_mode", "-2", CVAR_ARCHIVE_ND | CVAR_LATCH );
	cvar_t *cvFullscreen = Cvar_Get( "r_fullscreen", "1", CVAR_ARCHIVE_ND );
	cvar_t *cvNoborder = Cvar_Get( "r_noborder", "0", CVAR_ARCHIVE_ND | CVAR_LATCH );

	if( Cvar_VariableIntegerValue( "com_abnormalExit" ) )
	{
		Cvar_Set( "r_mode", va( "%d", R_MODE_FALLBACK ) );
		Cvar_Set( "r_fullscreen", "0" );
		Cvar_Set( "r_centerWindow", "0" );
		Cvar_Set( "com_abnormalExit", "0" );
	}

	Sys_GLimpInit();

	if (!SDL_WasInit(SDL_INIT_VIDEO))
	{
		if (SDL_Init(SDL_INIT_VIDEO) != 0)
		{
			Com_Error( ERR_FATAL, "SDL_Init( SDL_INIT_VIDEO ) FAILED (%s)", SDL_GetError());
			return;
		}

		Com_Printf( "SDL using driver \"%s\"\n", SDL_GetCurrentVideoDriver() );
	}

	if (VKimp_SetMode(cvMode->integer, cvFullscreen->integer, cvNoborder->integer) != RSERR_OK)
	{
		Sys_GLimpSafeInit();

		if (VKimp_SetMode(cvMode->integer, cvFullscreen->integer, qfalse) != RSERR_OK)
		{
			if( cvMode->integer != R_MODE_FALLBACK )
			{
				Com_Printf( "Setting r_mode %d failed, falling back on r_mode %d\n",
						cvMode->integer, R_MODE_FALLBACK );

				if (VKimp_SetMode(R_MODE_FALLBACK, qfalse, qfalse) != RSERR_OK)
				{
					Com_Error( ERR_FATAL, "VKimp_Init() - could not create Vulkan window" );
					return;
				}
			}
		}
	}

	// Fill in glConfig
	glimp_config->driverType = GLDRV_ICD;
	glimp_config->hardwareType = GLHW_GENERIC;
	glimp_config->deviceSupportsGamma = qfalse;  // VR headsets handle gamma

	// Vulkan doesn't use these GL strings, but fill in something useful
	Q_strncpyz(glimp_config->vendor_string, "Vulkan VR", sizeof(glimp_config->vendor_string));
	Q_strncpyz(glimp_config->renderer_string, "Q3VR Vulkan Renderer", sizeof(glimp_config->renderer_string));
	Q_strncpyz(glimp_config->version_string, "Vulkan 1.1", sizeof(glimp_config->version_string));
	glimp_config->extensions_string[0] = '\0';

	Cvar_Get( "r_availableModes", "", CVAR_ROM );

	// This depends on SDL_INIT_VIDEO, hence having it here
	IN_Init( SDL_window );
}


/*
===============
VKimp_Shutdown
===============
*/
void VKimp_Shutdown(qboolean unloadDLL)
{
	(void)unloadDLL;  // Not used - DLL management handled elsewhere

	IN_Shutdown();

	// [OpenXR] Destroy renderer and current XR session due to loss of Vulkan context,
	// will recreate it on next renderer init.
	VR_Engine* engine = VR_GetEngine();
	VR_DestroySessionInput(engine);
	VR_DestroyRenderer(engine);
	VR_LeaveVR(engine);

	// Reinitialize OpenXR instance and system, otherwise there might be visual
	// glitches/problems with swapchains when new session is (re)created.
	VR_Destroy(engine);
	VR_Init();

	if (SDL_window)
	{
		SDL_DestroyWindow(SDL_window);
		SDL_window = NULL;
	}

	SDL_QuitSubSystem(SDL_INIT_VIDEO);

	glimp_config = NULL;
}


/*
===============
VK_CreateSurface

Create a VkSurfaceKHR for the SDL window (for desktop mirror).
===============
*/
qboolean VK_CreateSurface(void *instance, void *pSurface)
{
	VkSurfaceKHR *surface = (VkSurfaceKHR *)pSurface;

	if (!SDL_window)
	{
		Com_Printf( "VK_CreateSurface: No SDL window\n");
		return qfalse;
	}

	if (!SDL_Vulkan_CreateSurface(SDL_window, (VkInstance)instance, surface))
	{
		Com_Printf( "VK_CreateSurface: SDL_Vulkan_CreateSurface failed: %s\n", SDL_GetError());
		return qfalse;
	}

	return qtrue;
}
