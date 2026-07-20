/*
===========================================================================
tr_qgl.c - GL function-pointer loading for the dlopen'd renderer.

The client-side glimp (sdl_glimp.c) has its own qgl* set; a renderer DLL
can't see it, so this defines separate storage and resolves it through
ri.GL_GetProcAddress. ARB/EXT extension pointers are stored here but loaded
in tr_extensions.c.
===========================================================================
*/
#include "tr_local.h"

int qglMajorVersion, qglMinorVersion;
int qglesMajorVersion, qglesMinorVersion;

// Legacy multitexture / compiled-vertex-array pointers (declared in qgl.h
// outside the QGL_*_PROCS lists); unused on modern GL, so NULL is correct.
void (APIENTRYP qglActiveTextureARB) (GLenum texture);
void (APIENTRYP qglClientActiveTextureARB) (GLenum texture);
void (APIENTRYP qglMultiTexCoord2fARB) (GLenum target, GLfloat s, GLfloat t);
void (APIENTRYP qglLockArraysEXT) (GLint first, GLsizei count);
void (APIENTRYP qglUnlockArraysEXT) (void);

// Storage for the qgl* pointers the renderer references (matches the extern
// set declared in renderercommon/qgl.h).
#define GLE( ret, name, ... ) name##proc * qgl##name;
QGL_1_1_PROCS;
QGL_DESKTOP_1_1_PROCS;
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

void QGL_InitRendererProcs( void )
{
	const char *version;

#define GLE( ret, name, ... ) qgl##name = (name##proc *) ri.GL_GetProcAddress( "gl" #name ); \
	if ( qgl##name == NULL ) { \
		ri.Printf( PRINT_ALL, "ERROR: Missing OpenGL function %s\n", "gl" #name ); \
	}

	// OpenGL 1.0 - glGetString is needed to read the version string.
	GLE( const GLubyte *, GetString, GLenum name )

	if ( !qglGetString ) {
		ri.Error( ERR_FATAL, "glGetString is NULL" );
	}

	version = (const char *)qglGetString( GL_VERSION );
	if ( !version ) {
		ri.Error( ERR_FATAL, "GL_VERSION is NULL" );
	}

	if ( Q_stricmpn( "OpenGL ES", version, 9 ) == 0 ) {
		char profile[6]; // ES, ES-CM, or ES-CL
		sscanf( version, "OpenGL %5s %d.%d", profile, &qglesMajorVersion, &qglesMinorVersion );
	} else {
		sscanf( version, "%d.%d", &qglMajorVersion, &qglMinorVersion );
	}

	// The renderer requires desktop OpenGL 2.0+; the client's glimp already
	// validated this and errors out otherwise.
	if ( QGL_VERSION_ATLEAST( 2, 0 ) ) {
		QGL_1_1_PROCS;
		QGL_DESKTOP_1_1_PROCS;
		QGL_1_3_PROCS;
		QGL_1_5_PROCS;
		QGL_2_0_PROCS;
	} else {
		ri.Error( ERR_FATAL, "Unsupported OpenGL Version (%s), OpenGL 2.0 is required", version );
	}

	if ( QGL_VERSION_ATLEAST( 3, 0 ) ) {
		QGL_3_0_PROCS;
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
		QGL_OVR_multiview_PROCS;
	}

#undef GLE
}

// Renderer-internal debug hook. The client-side glimp defines an identical
// no-op; the DLL needs its own copy because sdl_glimp.c is not linked in.
void GLimp_LogComment( char *comment )
{
}
