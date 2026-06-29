#version 450

// Desktop mirror fragment shader.
// SDR path (hdrMode==0): sample source, apply spec-constant gamma/obScale, output.
// HDR path (hdrMode==1): reconstruct above-paper-white highlights from the internal
//   scene+emissive buffers (not the clamped eye swapchain), apply gamma/overbright,
//   linearize sRGB, roll highlights toward the panel peak, output scRGB-linear FP16.
// SDR mirror path (hdrMode==2): source is final post-gamma SDR display content
//   (already-linear, sampled via UNORM view); scale to paper-white for
//   the scRGB-linear FP16 surface.

layout(set = 0, binding = 0) uniform sampler2DArray sourceTexture;
layout(set = 1, binding = 0) uniform sampler2DArray sceneTexture;    // pre-overbright scene base
layout(set = 2, binding = 0) uniform sampler2DArray emissiveTexture; // emissive highlight layer

layout(push_constant) uniform PushConstants {
	vec2 offset;
	vec2 scale;
	vec2 texCrop;
	int  eyeLayer;     // source array layer (0=left, 1=right)
	int  hdrMode;      // 0 = SDR, 1 = scRGB linear HDR, 2 = SDR mirror on scRGB surface
	float hdrGamma;    // 1/r_gamma for the HDR path
	float hdrObScale;  // overbright multiplier for the HDR path
	float paperWhite;  // nits SDR-white maps to
	float hdrPeak;     // nits display peak
	float hdrHighlight;// multiplier on the >1.0 headroom
} pc;

layout(location = 0) in vec2 texCoord;
layout(location = 0) out vec4 outColor;

layout(constant_id = 0) const float gamma = 1.0;
layout(constant_id = 1) const float obScale = 1.0;

vec3 sRGBtoLinear(vec3 c) {
	bvec3 cutoff = lessThanEqual(c, vec3(0.04045));
	vec3 lower = c / 12.92;
	vec3 higher = pow((c + vec3(0.055)) / vec3(1.055), vec3(2.4));
	return mix(higher, lower, cutoff);
}

// === BEGIN shared HDR core — keep byte-identical with
// trinity-engine code/renderervk/shaders/gamma.frag (the canonical copy) ===
// Reconstructs scRGB-linear HDR output from the SDR colour buffer and the emissive
// highlight layer. 1.0 == paper-white; result is scaled to scRGB (paper-white / 80).
vec3 hdrReconstruct( vec3 base, vec3 emissive,
                     float gamma, float obScale,
                     float paperWhite, float hdrPeak, float hdrHighlight )
{
	// Restore only where an additive emitter exceeded SDR white AND the colour
	// buffer is still clipped there. The emissive term excludes bloom, which
	// inflates base in halos but never writes the emissive layer (no fringing);
	// the base term restores 2D occlusion, since 2D composited on top darkens
	// base below the ceiling even where the emissive layer kept the emitter.
	float em = max( max( emissive.r, emissive.g ), emissive.b );
	float clip = smoothstep( 1.0, 1.1, em )
	           * smoothstep( 0.990, 1.0, max( max( base.r, base.g ), base.b ) );
	vec3 src = mix( base, max( base, emissive ), clip );

	vec3 lin = sRGBtoLinear( pow( src, vec3( gamma ) ) * obScale );

	// Hue-preserving highlights: scale all channels by one factor from the
	// brightest, rolling the headroom toward the panel peak.
	float m = max( max( lin.r, lin.g ), lin.b );
	if ( m > 1.0 )
	{
		float peak = max( hdrPeak / paperWhite, 1.0 );          // ceiling in paper-white units (>= paper-white)
		float boosted = 1.0 + (m - 1.0) * hdrHighlight;         // highlight push
		float rolled = peak * boosted / (peak - 1.0 + boosted); // soft asymptote at peak
		lin *= rolled / m;
	}

	return lin * (paperWhite / 80.0);
}
// === END shared HDR core ===

void main() {
	if (texCoord.x < 0.0 || texCoord.x > 1.0 || texCoord.y < 0.0 || texCoord.y > 1.0) {
		discard;
	}

	if ( pc.hdrMode == 1 )
	{
		// Reconstruct from the internal scene buffers, not the clamped eye swapchain.
		vec3 sceneBase = texture(sceneTexture,    vec3(texCoord, float(pc.eyeLayer))).rgb;
		vec3 emissive  = texture(emissiveTexture, vec3(texCoord, float(pc.eyeLayer))).rgb;
		outColor = vec4( hdrReconstruct( sceneBase, emissive, pc.hdrGamma, pc.hdrObScale,
			pc.paperWhite, pc.hdrPeak, pc.hdrHighlight ), 1.0 );
		return;
	}

	vec3 base = texture(sourceTexture, vec3(texCoord, float(pc.eyeLayer))).rgb;

	if ( pc.hdrMode == 2 )
	{
		// already-linear; scale to paper-white.
		outColor = vec4(base * (pc.paperWhite / 80.0), 1.0);
		return;
	}

	vec3 corrected = pow(base, vec3(gamma)) * obScale;
	outColor = vec4(corrected, 1.0);
}
