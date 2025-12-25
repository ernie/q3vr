#version 450
#extension GL_EXT_multiview : enable

// Per-eye MVP matrices via push constants (128 bytes = 2x mat4)
layout(push_constant) uniform Transform {
	mat4 mvp[2];
};

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec2 in_tex_coord;

layout(location = 0) out vec2 frag_pos;

out gl_PerVertex {
	vec4 gl_Position;
};

void main() {
	// Swizzle xzy for floor plane orientation (make it horizontal)
	gl_Position = mvp[gl_ViewIndex] * vec4(in_position.xzy, 1.0);
	frag_pos = in_tex_coord - vec2(0.5);
}
