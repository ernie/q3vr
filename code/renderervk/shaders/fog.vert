#version 450
#extension GL_EXT_multiview : enable

// Multiview fog vertex shader for VR stereo rendering
// Mono modelview via push constants; per-eye projection via ViewTransform UBO

// Mono modelview via push constants (64 bytes); per-eye projection lives in
// the ViewTransform UBO (set 0, binding 1), populated once per view.
layout(push_constant) uniform Transform {
	mat4 u_mv;              // mono modelview (V_head x M)
};

layout(set = 0, binding = 1) uniform ViewTransform {
	mat4 eyeProj[2];        // P_eye x E'_eye per view; equal slots when mono
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
	// Mono modelview (push constant) x per-eye projection (UBO)
	gl_Position = eyeProj[gl_ViewIndex] * (u_mv * vec4(in_position, 1.0));

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
