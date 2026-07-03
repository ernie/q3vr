#version 450
#extension GL_EXT_multiview : enable

// Flare visibility probe. This pipeline uses vk.pipeline_layout_storage (SSBO
// only, no eyeProj UBO binding), so the caller computes both eyes' clip-space
// probe positions on the CPU with the same eyeProj matrices the scene was
// rendered with and pushes them here; gl_ViewIndex picks the slot. A mono
// probe position would sample each view's depth buffer several degrees away
// from the flare (asymmetric-FOV offset + IPD parallax).
//
// The probe is a sub-pixel triangle, not a point: an NVIDIA driver bug
// hangs the GPU when a multiview POINT_LIST vertex shader both reads
// gl_ViewIndex and writes gl_PointSize; triangles avoid the trigger.
// gl_VertexIndex expands the pushed position into a ~2px triangle around
// the probe pixel.
//
// The SSBO reset (sampled = 0) lives on the CPU in RB_TestFlare: multiview
// gives no cross-view ordering between vertex and fragment invocations, so a
// vertex-stage reset could clobber the other view's fragment pass.
layout(push_constant) uniform Transform {
	vec4 clipPos[2];   // per-eye clip-space probe positions
	vec4 params;       // xy: clip-space size of ~2 pixels per unit w
};

layout(location = 0) in vec3 in_position; // unused; satisfies the pipeline's vertex input

out gl_PerVertex {
	vec4 gl_Position;
};

void main() {
	vec4 pos = clipPos[gl_ViewIndex];
	const vec2 offs[3] = vec2[3]( vec2(-0.5, -0.5), vec2(1.5, -0.5), vec2(-0.5, 1.5) );
	pos.xy += offs[gl_VertexIndex] * params.xy * pos.w;
	gl_Position = pos;
}
