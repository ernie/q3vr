#version 450

// 64 bytes. This pipeline uses vk.pipeline_layout_storage (SSBO only, no
// ViewTransform/eyeProj UBO binding), so it stays mono: the caller pushes the
// complete final transform directly (no separate per-eye projection stage).
layout(push_constant) uniform Transform {
	mat4 u_mv;
};

layout(set = 0, binding = 0) buffer SSBO {
	int sampled;
};

layout(location = 0) in vec3 in_position;

out gl_PerVertex {
	vec4 gl_Position;
	float gl_PointSize;
};

void main() {
	sampled = 0;
	gl_Position = u_mv * vec4(in_position, 1.0);
	gl_PointSize = 1.0;
}
