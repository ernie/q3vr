#version 450

// Overlay zoom fragment shader
// Samples from intermediate texture (single layer) and applies gamma correction.
// NOT multiview - renders to single-layer overlay swapchain.
//
// Gamma handling:
// - r_fbo=1: Source is linear HDR from FBO. Apply 1/r_gamma and overbright.
// - r_fbo=0: Source is sRGB-encoded from XR swapchain. Apply 1/2.2 to decode to linear.
//            Output goes to sRGB overlay swapchain which auto-encodes back to sRGB.

layout(set = 0, binding = 0) uniform sampler2D sourceTexture;

layout(location = 0) in vec2 texCoord;
layout(location = 0) out vec4 outColor;

// Specialization constants for gamma correction
layout(constant_id = 0) const float gamma = 1.0;
layout(constant_id = 1) const float obScale = 1.0;

void main() {
	vec4 color = texture(sourceTexture, texCoord);

	// Apply gamma correction and overbright scaling
	vec3 corrected = pow(color.rgb, vec3(gamma)) * obScale;

	outColor = vec4(corrected, 1.0);
}
