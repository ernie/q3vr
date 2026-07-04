#version 450
#extension GL_EXT_multiview : enable

// Mono modelview via push constants (64 bytes); per-eye projection lives in
// the ViewTransform UBO (set 0, binding 1), populated once at end-frame.
layout(push_constant) uniform Transform {
	mat4 u_mv;              // mono modelview (headView x model)
};

layout(set = 0, binding = 1) uniform ViewTransform {
	mat4 eyeProj[2];        // P_eye x eyeFromHead per view (XR meter space)
};

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec2 in_tex_coord;

layout(location = 0) out vec2 frag_tex_coord;

out gl_PerVertex {
	vec4 gl_Position;
};

void main() {
	gl_Position = eyeProj[gl_ViewIndex] * (u_mv * vec4(in_position, 1.0));
	// Flip V for Vulkan coordinate system (Y=0 at top)
	frag_tex_coord = vec2(in_tex_coord.x, 1.0 - in_tex_coord.y);
}
