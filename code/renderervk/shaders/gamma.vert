#version 450
#extension GL_EXT_multiview : enable

layout(location = 0) out vec2 frag_tex_coord;
layout(location = 1) out flat uint out_view_index;

const vec2 v[4] = vec2[4](
	vec2(-1.0f, 1.0f),
	vec2(-1.0f,-1.0f),
	vec2( 1.0f, 1.0f),
	vec2( 1.0f,-1.0f)
);

const vec2 t[4] = vec2[4](
	vec2( 0.0f, 1.0f),
	vec2( 0.0f, 0.0f),
	vec2( 1.0f, 1.0f),
	vec2( 1.0f, 0.0f)
);

out gl_PerVertex {
	vec4 gl_Position;
};

void main() {
	gl_Position = vec4( v[ gl_VertexIndex ], 0.0f, 1.0f );
	frag_tex_coord = t[ gl_VertexIndex ];
	out_view_index = gl_ViewIndex;
}
