#version 450
#extension GL_EXT_multiview : enable

// Per-eye MVP matrices via push constants (128 bytes = 2x mat4)
layout(push_constant) uniform Transform {
	mat4 mvp[2];
};

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec2 in_tex_coord;

layout(location = 0) out vec2 frag_tex_coord;

out gl_PerVertex {
	vec4 gl_Position;
};

void main() {
	gl_Position = mvp[gl_ViewIndex] * vec4(in_position, 1.0);
	// Flip V for Vulkan coordinate system (Y=0 at top)
	frag_tex_coord = vec2(in_tex_coord.x, 1.0 - in_tex_coord.y);
}
