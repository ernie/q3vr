#version 450
#extension GL_EXT_multiview : enable

// Multiview solid color vertex shader for VR stereo rendering
// Uses precomputed per-eye MVP matrices via push constants

// Per-eye MVP matrices via push constants (128 bytes = 2 x mat4)
// MVP is precomputed on CPU: mvp[eye] = proj[eye] * view[eye] * model
layout(push_constant) uniform Transform {
	mat4 mvp[2];
};

layout(location = 0) in vec3 in_position;

out gl_PerVertex {
	vec4 gl_Position;
};

void main() {
	// Per-eye MVP from push constants - precomputed on CPU
	gl_Position = mvp[gl_ViewIndex] * vec4(in_position, 1.0);
}
