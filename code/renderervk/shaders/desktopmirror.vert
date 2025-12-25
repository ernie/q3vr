#version 450

// Desktop mirror vertex shader
// Renders a fullscreen triangle with push constants for letterbox/pillarbox positioning.
// NOT multiview - renders to single-layer desktop swapchain.
//
// Scale interpretation:
// - scale < 1: Fit mode - quad is smaller than viewport, letterbox/pillarbox black bars
// - scale > 1: Fill mode - quad fills viewport, texture is zoomed/cropped

layout(push_constant) uniform PushConstants {
	vec2 offset;    // Letterbox offset in NDC [-1, 1]
	vec2 scale;     // Quad scale in NDC (0.5 = half width for both-eyes mode)
	vec2 texCrop;   // Texture crop factor (1.0 = full texture, <1 = crop to center region)
	int eyeLayer;   // Source array layer (0=left, 1=right)
} pc;

layout(location = 0) out vec2 texCoord;

out gl_PerVertex {
	vec4 gl_Position;
};

void main() {
	// Generate fullscreen triangle vertices from gl_VertexIndex (0, 1, 2)
	// This creates a triangle that covers the entire viewport:
	//   Vertex 0: (-1, -1) tex (0, 0)
	//   Vertex 1: ( 3, -1) tex (2, 0)
	//   Vertex 2: (-1,  3) tex (0, 2)
	vec2 baseTexCoord = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);

	// Apply texture cropping: texCrop < 1 means sample a smaller centered region
	// texCrop = 1.0: full texture (fit mode)
	// texCrop = 0.5: sample center 50% of texture (fill/crop mode)
	vec2 texOffset = (1.0 - pc.texCrop) * 0.5;  // Center the cropped region
	texCoord = baseTexCoord * pc.texCrop + texOffset;

	// Convert to NDC: [0,2] -> [-1,1] then apply offset/scale for letterbox
	vec2 pos = baseTexCoord * 2.0 - 1.0;
	pos = pos * pc.scale + pc.offset;

	gl_Position = vec4(pos, 0.0, 1.0);
}
