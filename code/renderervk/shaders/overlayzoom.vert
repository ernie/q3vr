#version 450

// Overlay zoom vertex shader
// Renders a fullscreen triangle for weapon zoom gamma correction.
// NOT multiview - renders to single-layer overlay swapchain.
// No push constants needed - always fullscreen with no offset/scale.

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
	texCoord = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);

	// Convert to NDC: [0,2] -> [-1,1]
	vec2 pos = texCoord * 2.0 - 1.0;

	gl_Position = vec4(pos, 0.0, 1.0);
}
