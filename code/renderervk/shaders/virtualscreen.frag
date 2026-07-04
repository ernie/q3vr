#version 450

layout(set = 1, binding = 0) uniform sampler2D virtualScreenTexture;

layout(location = 0) in vec2 frag_tex_coord;

layout(location = 0) out vec4 out_color;

// Specialization constants for gamma correction
// gamma: 1.0/r_gamma when FBO is active (source is linear), 2.2 when not (to decode sRGB)
// obScale: overbright scale (1 << overbrightBits) when FBO active, 1.0 when not
layout(constant_id = 0) const float gamma = 1.0;
layout(constant_id = 1) const float obScale = 1.0;

void main() {
	vec4 color = texture(virtualScreenTexture, frag_tex_coord);

	// Apply gamma correction:
	// - r_fbo=1: Source is linear HDR, apply 1/r_gamma and overbright
	// - r_fbo=0: Source is sRGB-encoded (sampled via UNORM view), decode to linear
	//            so the automatic sRGB conversion on output produces correct result
	// Proper sRGB transfer function: the linear segment near zero avoids
	// pow(x,1/2.2)'s infinite slope amplifying blit quantization noise.
	vec3 lo = color.rgb * 12.92;
	vec3 hi = 1.055 * pow(color.rgb, vec3(1.0 / 2.4)) - 0.055;
	vec3 corrected = mix(lo, hi, step(vec3(0.0031308), color.rgb)) * obScale;

	// Force fully opaque - virtual screen content should not be transparent
	out_color = vec4(corrected, 1.0);
}
