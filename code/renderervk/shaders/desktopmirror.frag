#version 450

// Desktop mirror fragment shader
// Samples from intermediate texture (2-layer array) and applies gamma correction.
// NOT multiview - renders to single-layer desktop swapchain.
//
// Gamma handling:
// - r_fbo=1: Source is linear HDR from FBO. Apply 1/r_gamma and overbright.
// - r_fbo=0: Source is sRGB-encoded from XR swapchain. Apply 1/2.2 to decode to linear.
//            Output goes to sRGB desktop swapchain which auto-encodes back to sRGB.

layout(set = 0, binding = 0) uniform sampler2DArray sourceTexture;

layout(push_constant) uniform PushConstants {
	vec2 offset;
	vec2 scale;
	vec2 texCrop;
	int eyeLayer;   // Source array layer (0=left, 1=right)
} pc;

layout(location = 0) in vec2 texCoord;
layout(location = 0) out vec4 outColor;

// Specialization constants for gamma correction
layout(constant_id = 0) const float gamma = 1.0;
layout(constant_id = 1) const float obScale = 1.0;

void main() {
	// Discard fragments outside valid texture coordinate range [0,1]
	// This ensures letterbox/pillarbox areas show black from render pass clear
	if (texCoord.x < 0.0 || texCoord.x > 1.0 || texCoord.y < 0.0 || texCoord.y > 1.0) {
		discard;
	}

	vec4 color = texture(sourceTexture, vec3(texCoord, float(pc.eyeLayer)));

	// Apply gamma correction and overbright scaling
	vec3 corrected = pow(color.rgb, vec3(gamma)) * obScale;

	outColor = vec4(corrected, 1.0);
}
