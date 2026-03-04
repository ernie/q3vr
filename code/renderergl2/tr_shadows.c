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
#include "tr_local.h"


/*

  for a projection shadow:

  point[x] += light vector * ( z - shadow plane )
  point[y] +=
  point[z] = shadow plane

  1 0 light[x] / light[z]

*/

typedef struct {
	int		i2;
	int		facing;
} edgeDef_t;

#define	MAX_EDGE_DEFS	32

static	edgeDef_t	edgeDefs[SHADER_MAX_VERTEXES][MAX_EDGE_DEFS];
static	int			numEdgeDefs[SHADER_MAX_VERTEXES];
static	int			facing[SHADER_MAX_INDEXES/3];
static	int			numLitTris;
static	int			litTriIndexes[SHADER_MAX_INDEXES];

static void R_AddEdgeDef( int i1, int i2, int f ) {
	int		c;

	c = numEdgeDefs[ i1 ];
	if ( c == MAX_EDGE_DEFS ) {
		return;		// overflow
	}
	edgeDefs[ i1 ][ c ].i2 = i2;
	edgeDefs[ i1 ][ c ].facing = f;

	numEdgeDefs[ i1 ]++;
}

/*
=================
R_CalcShadowEdges

Build indexed quads for silhouette edges into tess.indexes.
Projected vertices are at tess.xyz[i + tess.numVertexes].
Ported from renderervk (OpenGL winding order).
=================
*/
static void R_CalcShadowEdges( void ) {
	qboolean sil_edge;
	int		i;
	int		c, c2;
	int		j, k;
	int		i2;

	tess.numIndexes = 0;

	// an edge is NOT a silhouette edge if its face doesn't face the light,
	// or if it has a reverse paired edge that also faces the light.
	// A well behaved polyhedron would have exactly two faces for each edge,
	// but lots of models have dangling edges or overfanned edges
	for ( i = 0; i < tess.numVertexes; i++ ) {
		c = numEdgeDefs[ i ];
		for ( j = 0; j < c; j++ ) {
			if ( !edgeDefs[ i ][ j ].facing ) {
				continue;
			}

			sil_edge = qtrue;
			i2 = edgeDefs[ i ][ j ].i2;
			c2 = numEdgeDefs[ i2 ];
			for ( k = 0; k < c2; k++ ) {
				if ( edgeDefs[ i2 ][ k ].i2 == i && edgeDefs[ i2 ][ k ].facing ) {
					sil_edge = qfalse;
					break;
				}
			}

			// if it doesn't share the edge with another front facing
			// triangle, it is a sil edge
			if ( sil_edge ) {
				if ( tess.numIndexes > ARRAY_LEN( tess.indexes ) - 6 ) {
					i = tess.numVertexes;
					break;
				}
				tess.indexes[ tess.numIndexes + 0 ] = i;
				tess.indexes[ tess.numIndexes + 1 ] = i + tess.numVertexes;
				tess.indexes[ tess.numIndexes + 2 ] = i2;
				tess.indexes[ tess.numIndexes + 3 ] = i2;
				tess.indexes[ tess.numIndexes + 4 ] = i + tess.numVertexes;
				tess.indexes[ tess.numIndexes + 5 ] = i2 + tess.numVertexes;
				tess.numIndexes += 6;
			}
		}
	}
}

/*
=================
RB_ShadowTessEnd

triangleFromEdge[ v1 ][ v2 ]


  set triangle from edge( v1, v2, tri )
  if ( facing[ triangleFromEdge[ v1 ][ v2 ] ] && !facing[ triangleFromEdge[ v2 ][ v1 ] ) {
  }
=================
*/
void RB_ShadowTessEnd( void ) {
	int		i;
	int		numTris;
	vec3_t	lightDir;
	GLboolean rgba[4];

	if ( glConfig.stencilBits < 4 ) {
		return;
	}

	VectorCopy( backEnd.currentEntity->modelLightDir, lightDir );

	// project vertexes away from light direction
	for ( i = 0; i < tess.numVertexes; i++ ) {
		VectorMA( tess.xyz[i], -r_shadowDistance->value, lightDir, tess.xyz[i+tess.numVertexes] );
	}

	// decide which triangles face the light
	Com_Memset( numEdgeDefs, 0, tess.numVertexes * sizeof( numEdgeDefs[0] ) );

	numTris = tess.numIndexes / 3;
	for ( i = 0; i < numTris; i++ ) {
		int		i1, i2, i3;
		vec3_t	d1, d2, normal;
		float	*v1, *v2, *v3;
		float	d;

		i1 = tess.indexes[ i*3 + 0 ];
		i2 = tess.indexes[ i*3 + 1 ];
		i3 = tess.indexes[ i*3 + 2 ];

		v1 = tess.xyz[ i1 ];
		v2 = tess.xyz[ i2 ];
		v3 = tess.xyz[ i3 ];

		VectorSubtract( v2, v1, d1 );
		VectorSubtract( v3, v1, d2 );
		CrossProduct( d1, d2, normal );

		d = DotProduct( normal, lightDir );
		facing[ i ] = ( d > 0 ) ? 1 : 0;

		// create the edges
		R_AddEdgeDef( i1, i2, facing[ i ] );
		R_AddEdgeDef( i2, i3, facing[ i ] );
		R_AddEdgeDef( i3, i1, facing[ i ] );
	}

	// save lit-facing triangle indices for back cap generation
	numLitTris = 0;
	for ( i = 0; i < numTris; i++ ) {
		if ( facing[i] ) {
			litTriIndexes[ numLitTris*3 + 0 ] = tess.indexes[ i*3 + 0 ];
			litTriIndexes[ numLitTris*3 + 1 ] = tess.indexes[ i*3 + 1 ];
			litTriIndexes[ numLitTris*3 + 2 ] = tess.indexes[ i*3 + 2 ];
			numLitTris++;
		}
	}

	R_CalcShadowEdges();

	// append back cap: lit-facing triangles at extruded positions, reversed winding
	for ( i = 0; i < numLitTris; i++ ) {
		if ( tess.numIndexes > ARRAY_LEN( tess.indexes ) - 3 )
			break;
		tess.indexes[ tess.numIndexes + 0 ] = litTriIndexes[ i*3 + 0 ] + tess.numVertexes;
		tess.indexes[ tess.numIndexes + 1 ] = litTriIndexes[ i*3 + 2 ] + tess.numVertexes;
		tess.indexes[ tess.numIndexes + 2 ] = litTriIndexes[ i*3 + 1 ] + tess.numVertexes;
		tess.numIndexes += 3;
	}

	// double vertex count: projected vertices follow originals in tess.xyz
	tess.numVertexes *= 2;

	// upload position-only (shadow rendering doesn't need normals/texcoords/etc)
	RB_UpdateTessVao( ATTR_POSITION );

	// bind textureColorShader for stencil-only rendering
	{
		shaderProgram_t *sp = &tr.textureColorShader;
		vec4_t color;

		GLSL_BindProgram( sp );
		GLSL_SetUniformMat4( sp, UNIFORM_MODELMATRIX, glState.modelMatrix );
		GLSL_BindBuffers( sp );
		VectorSet4( color, 0.2f, 0.2f, 0.2f, 1.0f );
		GLSL_SetUniformVec4( sp, UNIFORM_COLOR, color );
		GLSL_SetUniformInt( sp, UNIFORM_ALPHATEST, 0 );
	}

	GL_BindToTMU( tr.whiteImage, TB_COLORMAP );
	GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO );

	// don't write to the color buffer
	qglGetBooleanv( GL_COLOR_WRITEMASK, rgba );
	qglColorMask( GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE );

	qglEnable( GL_STENCIL_TEST );
	qglStencilFunc( GL_EQUAL, 0, 0x80 );   // skip entity-marked pixels (bit 7 set)
	qglStencilMask( 0x7F );                 // only write shadow count to bits 0-6

	// mirrors reverse triangle winding, so swap cull modes
	{
		cullType_t backCull  = backEnd.viewParms.isMirror ? CT_FRONT_SIDED : CT_BACK_SIDED;
		cullType_t frontCull = backEnd.viewParms.isMirror ? CT_BACK_SIDED  : CT_FRONT_SIDED;

		// draw back-sided (stencil increment)
		GL_Cull( backCull );
		qglStencilOp( GL_KEEP, GL_KEEP, GL_INCR );
		R_DrawElements( tess.numIndexes, tess.firstIndex );

		// draw front-sided (stencil decrement)
		GL_Cull( frontCull );
		qglStencilOp( GL_KEEP, GL_KEEP, GL_DECR );
		R_DrawElements( tess.numIndexes, tess.firstIndex );
	}

	qglStencilMask( 0xFF );

	// reenable writing to the color buffer
	qglColorMask( rgba[0], rgba[1], rgba[2], rgba[3] );

	qglDisable( GL_STENCIL_TEST );

	backEnd.doneShadows = qtrue;

	tess.numIndexes = 0;
	tess.numVertexes /= 2;
}


/*
=================
RB_ShadowFinish

Darken everything that is in a shadow volume.
We have to delay this until everything has been shadowed,
because otherwise shadows from different body parts would
overlap and double darken.
=================
*/
void RB_ShadowFinish( void ) {
	mat4_t oldModelMatrix;

	if ( !backEnd.doneShadows ) {
		return;
	}

	backEnd.doneShadows = qfalse;

	if ( r_shadows->integer != 2 ) {
		return;
	}
	if ( glConfig.stencilBits < 4 ) {
		return;
	}

	// save model matrix
	Mat4Copy( glState.modelMatrix, oldModelMatrix );

	// set identity model matrix
	{
		mat4_t identityMatrix;
		Mat4Identity( identityMatrix );
		GL_SetModelMatrix( identityMatrix );
	}

	// build fullscreen quad in ortho coordinates (0,0)-(vidWidth,vidHeight)
	{
		float w = (float)glConfig.vidWidth;
		float h = (float)glConfig.vidHeight;

		tess.numVertexes = 0;
		tess.numIndexes = 0;
		tess.firstIndex = 0;

		VectorSet4( tess.xyz[0], 0, 0, 0.5f, 1.0f );
		VectorSet4( tess.xyz[1], w, 0, 0.5f, 1.0f );
		VectorSet4( tess.xyz[2], w, h, 0.5f, 1.0f );
		VectorSet4( tess.xyz[3], 0, h, 0.5f, 1.0f );
		tess.numVertexes = 4;

		tess.indexes[0] = 0;
		tess.indexes[1] = 1;
		tess.indexes[2] = 2;
		tess.indexes[3] = 0;
		tess.indexes[4] = 2;
		tess.indexes[5] = 3;
		tess.numIndexes = 6;
	}

	// upload geometry
	RB_UpdateTessVao( ATTR_POSITION );

	// bind textureColorShader with fullscreen ortho projection
	{
		shaderProgram_t *sp = &tr.textureColorShader;
		vec4_t color;
		mat4_t identityMatrix;

		Mat4Identity( identityMatrix );

		GLSL_BindProgram( sp );
		GLSL_SetUniformMat4( sp, UNIFORM_MODELMATRIX, identityMatrix );
		GLSL_BindFullscreenOrthoBuffers( sp );
		VectorSet4( color, 0.6f, 0.6f, 0.6f, 1.0f );
		GLSL_SetUniformVec4( sp, UNIFORM_COLOR, color );
		GLSL_SetUniformInt( sp, UNIFORM_ALPHATEST, 0 );
	}

	GL_BindToTMU( tr.whiteImage, TB_COLORMAP );

	GL_Cull( CT_TWO_SIDED );

	qglEnable( GL_STENCIL_TEST );
	qglStencilFunc( GL_NOTEQUAL, 0, 0x7F );  // check shadow bits 0-6 only

	GL_State( GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO );

	qglDrawElements( GL_TRIANGLES, 6, GL_INDEX_TYPE, BUFFER_OFFSET(0) );

	qglDisable( GL_STENCIL_TEST );

	// restore model matrix
	GL_SetModelMatrix( oldModelMatrix );

	tess.numIndexes = 0;
	tess.numVertexes = 0;
	tess.firstIndex = 0;
}


/*
=================
RB_ProjectionShadowDeform

=================
*/
void RB_ProjectionShadowDeform( void ) {
	float	*xyz;
	int		i;
	float	h;
	vec3_t	ground;
	vec3_t	light;
	float	groundDist;
	float	d;
	vec3_t	lightDir;

	xyz = ( float * ) tess.xyz;

	ground[0] = backEnd.or.axis[0][2];
	ground[1] = backEnd.or.axis[1][2];
	ground[2] = backEnd.or.axis[2][2];

	groundDist = backEnd.or.origin[2] - backEnd.currentEntity->e.shadowPlane;

	VectorCopy( backEnd.currentEntity->modelLightDir, lightDir );
	d = DotProduct( lightDir, ground );
	// don't let the shadows get too long or go negative
	if ( d < 0.5 ) {
		VectorMA( lightDir, (0.5 - d), ground, lightDir );
		d = DotProduct( lightDir, ground );
	}
	d = 1.0 / d;

	light[0] = lightDir[0] * d;
	light[1] = lightDir[1] * d;
	light[2] = lightDir[2] * d;

	for ( i = 0; i < tess.numVertexes; i++, xyz += 4 ) {
		h = DotProduct( xyz, ground ) + groundDist;

		xyz[0] -= light[0] * h;
		xyz[1] -= light[1] * h;
		xyz[2] -= light[2] * h;
	}
}
