/*
 * vr_vk_virtual_screen.c - Vulkan virtual screen implementation
 *
 * Virtual screen is used for menus, console, and first-person follow mode.
 * The game renders to a texture which is then displayed as a curved 3D quad.
 *
 * Anchor state and pose math live in vrcommon/vr_virtual_screen.c; this
 * layer owns only the Vulkan-facing split-transform query.
 */

#include "vr_vk_virtual_screen.h"

#include "../vrcommon/common/xr_linear.h"
#include "../vrcommon/vr_gameplay.h"
#include "../vrcommon/vr_virtual_screen.h"

// Per-frame view data from vr_vk_renderer.c
extern XrView views[2];
extern uint32_t viewCount;


void VRVK_VirtualScreen_Init(void)
{
	VR_VirtualScreen_ResetPosition();
}


void VRVK_VirtualScreen_Destroy(void)
{
	// Nothing to destroy - Vulkan resources are managed by the renderer
}


/*
==================
VR_GetVirtualScreenMatrices

Query function for renderer to pull virtual screen state.
Returns qtrue if virtual screen should be rendered.

Returns the split transform: one eyeProj pair (P_eye * eyeFromHead, XR meter
space) shared by both meshes, plus a mono model-view (headView * model) per
mesh. Exact factorization of the former per-eye MVPs:
P_e * V_e * M == (P_e * (V_e * headPose)) * (inverse(headPose) * M).
==================
*/
qboolean VR_GetVirtualScreenMatrices(float eyeProj[2][16], float screenModelView[16], float floorModelView[16])
{
	int e;

	// Check if we should render the virtual screen
	if (!VR_Gameplay_ShouldRenderInVirtualScreen())
	{
		return qfalse;
	}

	// Get per-eye poses
	XrPosef* left = &views[0].pose;
	XrPosef* right = &views[viewCount > 1 ? viewCount - 1 : 0].pose;

	// Compute centered head pose (midpoint between eyes)
	XrPosef centeredHead;
	VR_VirtualScreen_ComputeCenteredHeadPose(&centeredHead, views, viewCount);

	// Head pose as a rigid transform and its inverse (the mono view)
	XrVector3f unitScale = { 1.0f, 1.0f, 1.0f };
	XrMatrix4x4f headPoseMat, headView;
	XrMatrix4x4f_CreateTranslationRotationScale(&headPoseMat, &centeredHead.position, &centeredHead.orientation, &unitScale);
	XrMatrix4x4f_InvertRigidBody(&headView, &headPoseMat);

	// Build view matrices
	XrMatrix4x4f view[2];
	VR_VirtualScreen_GetViewMatrix(&view[0], &left->position, &left->orientation);
	VR_VirtualScreen_GetViewMatrix(&view[1], &right->position, &right->orientation);

	// Build projection matrices (Vulkan coordinate system)
	XrMatrix4x4f projection[2];
	XrMatrix4x4f_CreateProjectionFov(&projection[0], GRAPHICS_VULKAN, views[0].fov, 0.01f, 100.0f);
	if (viewCount > 1) {
		XrMatrix4x4f_CreateProjectionFov(&projection[1], GRAPHICS_VULKAN, views[viewCount - 1].fov, 0.01f, 100.0f);
	} else {
		projection[1] = projection[0];
	}

	// eyeProj[e] = P_e * eyeFromHead_e; eyeFromHead maps head space to eye space
	for (e = 0; e < 2; e++) {
		XrMatrix4x4f eyeFromHead, ep;
		XrMatrix4x4f_Multiply(&eyeFromHead, &view[e], &headPoseMat);
		XrMatrix4x4f_Multiply(&ep, &projection[e], &eyeFromHead);
		memcpy(eyeProj[e], ep.m, sizeof(ep.m));
	}

	// Mono model-views: headView * model
	{
		XrMatrix4x4f screenModel, floorModel, mv;

		VR_VirtualScreen_GetModelMatrix(&screenModel, &centeredHead);
		VR_VirtualScreen_GetFloorModelMatrix(&floorModel);

		XrMatrix4x4f_Multiply(&mv, &headView, &screenModel);
		memcpy(screenModelView, mv.m, sizeof(mv.m));

		XrMatrix4x4f_Multiply(&mv, &headView, &floorModel);
		memcpy(floorModelView, mv.m, sizeof(mv.m));
	}

	return qtrue;
}
