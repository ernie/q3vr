#version 450
#extension GL_EXT_multiview : enable

// Multiview solid color vertex shader for VR stereo rendering
// Mono modelview via push constants; per-eye projection via ViewTransform UBO

// Mono modelview via push constants (64 bytes); per-eye projection lives in
// the ViewTransform UBO (set 0, binding 1), populated once per view.
layout(push_constant) uniform Transform {
	mat4 u_mv;              // mono modelview (V_head x M)
};

layout(set = 0, binding = 1) uniform ViewTransform {
	mat4 eyeProj[2];        // P_eye x E'_eye per view; equal slots when mono
};

layout(location = 0) in vec3 in_position;

out gl_PerVertex {
	vec4 gl_Position;
};

void main() {
	// Mono modelview (push constant) x per-eye projection (UBO)
	gl_Position = eyeProj[gl_ViewIndex] * (u_mv * vec4(in_position, 1.0));
}
