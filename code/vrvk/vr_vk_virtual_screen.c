/*
 * vr_vk_virtual_screen.c - Vulkan virtual screen implementation
 *
 * Virtual screen is used for menus, console, and first-person follow mode.
 * The game renders to a texture which is then displayed as a curved 3D quad.
 */

#include "vr_vk_virtual_screen.h"

#include "../vrcommon/common/xr_linear.h"
#include "../vrcommon/vr_clientinfo.h"
#include "../vrcommon/vr_gameplay.h"

extern vr_clientinfo_t vr;
extern cvar_t *vr_virtualScreenShape;

// Per-frame view data from vr_vk_renderer.c
extern XrView views[2];
extern uint32_t viewCount;

// Forward declaration for quaternion to angles conversion
extern void QuatToYawPitchRoll(XrQuaternionf q, vec3_t rotation, vec3_t out);

// Virtual screen shape enum
typedef enum {
	CURVED = 0,
	FLAT = 1,
} virtualScreenShape_t;

// Current virtual screen orientation (for tracking yaw)
static XrQuaternionf lastKnownVirtualScreenOrientation = { 0.0f, 0.0f, 0.0f, 1.0f };

// Position tracking state
static int positionsInitialized = 0;
static int updateTarget = 1;
static XrVector3f targetPosition;
static XrQuaternionf targetRotation;
static XrVector3f currentPosition;
static XrQuaternionf currentRotation;


/*
==================
GetCurrentVirtualScreenPositionAndRotation

Compute the virtual screen's position and rotation based on HMD pose.
The screen should face the player and be positioned at a comfortable distance.
==================
*/
static void GetCurrentVirtualScreenPositionAndRotation(XrPosef* headPose,
	XrVector3f* translation, XrQuaternionf* rotation)
{
	// Position the screen at a comfortable distance from the player
	const float virtualScreenDistance = 3.0f;

	if (!positionsInitialized || updateTarget)
	{
		// Use the head pose to position the screen
		XrVector3f forward = { 0.0f, 0.0f, -1.0f };
		XrVector3f rotatedForward;
		XrQuaternionf_RotateVector3f(&rotatedForward, &headPose->orientation, &forward);

		// Place screen in front of the user at virtual screen distance
		targetPosition.x = headPose->position.x + rotatedForward.x * virtualScreenDistance;
		targetPosition.y = headPose->position.y + rotatedForward.y * virtualScreenDistance;
		targetPosition.z = headPose->position.z + rotatedForward.z * virtualScreenDistance;

		// Screen should face the player
		targetRotation = headPose->orientation;

		if (!positionsInitialized)
		{
			currentPosition = targetPosition;
			currentRotation = targetRotation;
			positionsInitialized = 1;
		}
		updateTarget = 0;
	}

	// Smooth interpolation toward target
	const float lerpSpeed = 0.1f;
	currentPosition.x += (targetPosition.x - currentPosition.x) * lerpSpeed;
	currentPosition.y += (targetPosition.y - currentPosition.y) * lerpSpeed;
	currentPosition.z += (targetPosition.z - currentPosition.z) * lerpSpeed;

	// Lerp rotation (normalize after to approximate slerp)
	XrQuaternionf_Lerp(&currentRotation, &currentRotation, &targetRotation, lerpSpeed);
	XrQuaternionf_Normalize(&currentRotation);

	*translation = currentPosition;
	*rotation = currentRotation;
}


/*
==================
_VR_GetVirtualScreenModelMatrix

Create model matrix for the virtual screen cylinder.
==================
*/
static void _VR_GetVirtualScreenModelMatrix(XrMatrix4x4f* model, XrPosef* headPose)
{
	// Base 4:3 aspect ratio for the virtual screen content
	float aspectRatioCoeff = 3.0f / 4.0f;

	// Compensate for non-square framebuffer aspect ratio
	if (vr.fov_x > 0.0f && vr.fov_y > 0.0f)
	{
		float framebufferAspect = vr.fov_y / vr.fov_x;
		aspectRatioCoeff *= framebufferAspect;
	}

	XrVector3f translation;
	XrQuaternionf rotation;
	GetCurrentVirtualScreenPositionAndRotation(headPose, &translation, &rotation);
	XrVector3f scale = { 3.0f, 3.0f * aspectRatioCoeff, 3.0f };

	lastKnownVirtualScreenOrientation = rotation;

	// Lower the screen slightly
	translation.y -= 0.5f;

	XrMatrix4x4f_CreateTranslationRotationScale(model, &translation, &rotation, &scale);
}


/*
==================
_VR_GetVirtualScreenFloorModelMatrix

Create model matrix for the floor grid quad.
==================
*/
static void _VR_GetVirtualScreenFloorModelMatrix(XrMatrix4x4f* model)
{
	XrVector3f translation = { 0.0f, 0.0f, 0.0f };
	XrQuaternionf rotation = { 0.0f, 0.0f, 0.0f, 1.0f };
	XrVector3f scale = { 30.0f, 30.0f, -30.0f };

	// Align the floor with actual stage space instead of initial HMD rotation
	XrVector3f upAxis = { 0.0f, 1.0f, 0.0f };
	XrQuaternionf_CreateFromAxisAngle(&rotation, &upAxis, -vr.recenterYaw);

	XrMatrix4x4f_CreateTranslationRotationScale(model, &translation, &rotation, &scale);
}


/*
==================
_VR_GetVirtualScreenViewMatrix

Create view matrix from eye position and orientation.
==================
*/
static void _VR_GetVirtualScreenViewMatrix(XrMatrix4x4f* result,
	XrVector3f* translation, XrQuaternionf* rotation)
{
	XrMatrix4x4f rotationMatrix, translationMatrix, viewMatrix;
	XrMatrix4x4f_CreateFromQuaternion(&rotationMatrix, rotation);
	XrMatrix4x4f_CreateTranslation(&translationMatrix, translation->x, translation->y, translation->z);
	XrMatrix4x4f_Multiply(&viewMatrix, &translationMatrix, &rotationMatrix);
	XrMatrix4x4f_Invert(result, &viewMatrix);
}


void VR_VirtualScreen_Init(void)
{
	positionsInitialized = 0;
	updateTarget = 1;
}


void VR_VirtualScreen_Destroy(void)
{
	// Nothing to destroy - Vulkan resources are managed by the renderer
}


void VR_VirtualScreen_ResetPosition(void)
{
	positionsInitialized = 0;
	updateTarget = 1;
}


/*
==================
VR_GetVirtualScreenMVP

Query function for renderer to pull virtual screen state.
Returns qtrue if virtual screen should be rendered.
Fills in precomputed per-eye MVP matrices for screen and floor.

Each MVP array is 2 * 64 bytes = 128 bytes (two mat4 for left/right eyes).
==================
*/
qboolean VR_GetVirtualScreenMVP(float screenMVP[2][16], float floorMVP[2][16])
{
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
	centeredHead.position.x = (left->position.x + right->position.x) * 0.5f;
	centeredHead.position.y = (left->position.y + right->position.y) * 0.5f;
	centeredHead.position.z = (left->position.z + right->position.z) * 0.5f;
	centeredHead.orientation = left->orientation;

	// Build view matrices
	XrMatrix4x4f view[2];
	_VR_GetVirtualScreenViewMatrix(&view[0], &left->position, &left->orientation);
	_VR_GetVirtualScreenViewMatrix(&view[1], &right->position, &right->orientation);

	// Build projection matrices (Vulkan coordinate system)
	XrMatrix4x4f projection[2];
	XrMatrix4x4f_CreateProjectionFov(&projection[0], GRAPHICS_VULKAN, views[0].fov, 0.01f, 100.0f);
	if (viewCount > 1) {
		XrMatrix4x4f_CreateProjectionFov(&projection[1], GRAPHICS_VULKAN, views[viewCount - 1].fov, 0.01f, 100.0f);
	} else {
		projection[1] = projection[0];
	}

	// Virtual screen model matrix
	XrMatrix4x4f screenModel;
	_VR_GetVirtualScreenModelMatrix(&screenModel, &centeredHead);

	// Floor model matrix
	XrMatrix4x4f floorModel;
	_VR_GetVirtualScreenFloorModelMatrix(&floorModel);

	// Compute MVP = Projection * View * Model for each eye
	XrMatrix4x4f viewModel, mvp;

	// Screen MVP for left eye
	XrMatrix4x4f_Multiply(&viewModel, &view[0], &screenModel);
	XrMatrix4x4f_Multiply(&mvp, &projection[0], &viewModel);
	memcpy(screenMVP[0], mvp.m, 16 * sizeof(float));

	// Screen MVP for right eye
	XrMatrix4x4f_Multiply(&viewModel, &view[1], &screenModel);
	XrMatrix4x4f_Multiply(&mvp, &projection[1], &viewModel);
	memcpy(screenMVP[1], mvp.m, 16 * sizeof(float));

	// Floor MVP for left eye
	XrMatrix4x4f_Multiply(&viewModel, &view[0], &floorModel);
	XrMatrix4x4f_Multiply(&mvp, &projection[0], &viewModel);
	memcpy(floorMVP[0], mvp.m, 16 * sizeof(float));

	// Floor MVP for right eye
	XrMatrix4x4f_Multiply(&viewModel, &view[1], &floorModel);
	XrMatrix4x4f_Multiply(&mvp, &projection[1], &viewModel);
	memcpy(floorMVP[1], mvp.m, 16 * sizeof(float));

	return qtrue;
}


float VR_VirtualScreen_GetCurrentYaw(void)
{
	vec3_t rotation = { 0, 0, 0 };
	vec3_t yawPitchRoll = { 0, 0, 0 };
	QuatToYawPitchRoll(lastKnownVirtualScreenOrientation, rotation, yawPitchRoll);
	return yawPitchRoll[YAW];
}
