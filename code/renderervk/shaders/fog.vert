#version 450
#extension GL_EXT_multiview : enable

// Multiview fog vertex shader for VR stereo rendering
// Uses precomputed per-eye MVP matrices via push constants

// Per-eye MVP matrices via push constants (128 bytes = 2 x mat4)
// MVP is precomputed on CPU: mvp[eye] = proj[eye] * view[eye] * model
layout(push_constant) uniform Transform {
	mat4 mvp[2];
};

// Fog UBO at set 0, binding 0 (standard layout)
layout(set = 0, binding = 0) uniform UBO {
	// light/env parameters (unused in fog shader but part of layout):
	vec4 eyePos;
	vec4 lightPos;
	vec4 lightColor;
	vec4 lightVector;
	// fog parameters:
	vec4 fogDistanceVector;		// vertex
	vec4 fogDepthVector;		// vertex
	vec4 fogEyeT;				// vertex
	vec4 fogColor;				// fragment
};

layout(location = 0) in vec3 in_position;

layout(location = 4) out vec2 fog_tex_coord;

out gl_PerVertex {
	vec4 gl_Position;
};

void main() {
	// Per-eye MVP from push constants - precomputed on CPU
	gl_Position = mvp[gl_ViewIndex] * vec4(in_position, 1.0);

	// Fog calculations
	float s = dot(in_position, fogDistanceVector.xyz) + fogDistanceVector.w;
	float t = dot(in_position, fogDepthVector.xyz) + fogDepthVector.w;

	if ( fogEyeT.y == 1.0 ) {
		if ( t < 0.0 ) {
			t = 1.0 / 32.0;
		} else {
			t = 31.0 / 32.0;
		}
	} else {
		if ( t < 1.0 ) {
			t = 1.0 / 32.0;
		} else {
			t = 1.0 / 32.0 + (30.0 / 32.0 * t) / ( t - fogEyeT.x );
		}
	}

	fog_tex_coord = vec2(s, t);
}
