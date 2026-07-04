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
extern cvar_t *vr_virtualScreenMode;

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
static XrVector3f currentPosition;
static XrQuaternionf currentRotation;

// Screen anchor distance from the head, meters
static const float targetDistance = 3.0f;


/*
==================
GetPositionInFront

Head forward projected onto the horizontal plane, at the given distance and
the head's own height - the screen anchors upright at eye level regardless
of head pitch.
==================
*/
static XrVector3f GetPositionInFront(const XrPosef* pose, float distance)
{
	XrVector3f forward = { 0.0f, 0.0f, -1.0f };
	XrVector3f rotatedForward;
	XrQuaternionf_RotateVector3f(&rotatedForward, &pose->orientation, &forward);

	// Project to XZ plane (remove Y component to keep same height)
	rotatedForward.y = 0.0f;
	XrVector3f_Normalize(&rotatedForward);

	XrVector3f result = {
		pose->position.x + rotatedForward.x * distance,
		pose->position.y,
		pose->position.z + rotatedForward.z * distance
	};
	return result;
}


/*
==================
YawFacingQuaternion

Yaw-only rotation turning the screen toward the head - keeps the screen
upright instead of inheriting the anchoring head pose's pitch/roll.
==================
*/
static XrQuaternionf YawFacingQuaternion(const XrVector3f* from, const XrVector3f* to)
{
	float dx = to->x - from->x;
	float dz = to->z - from->z;

	float len = sqrtf(dx * dx + dz * dz);
	if (len < 1e-6f)
	{
		return (XrQuaternionf){ 0.0f, 0.0f, 0.0f, 1.0f };
	}
	dx /= len;
	dz /= len;

	float yaw = atan2f(dx, dz);
	float half = yaw * 0.5f;
	return (XrQuaternionf){ 0.0f, sinf(half), 0.0f, cosf(half) };
}


/*
==================
EnsureNewPositionInExpectedDistance

Clamp a drifting screen position back onto the targetDistance sphere around
the head so follow-mode drift never changes the apparent screen size.
==================
*/
static void EnsureNewPositionInExpectedDistance(const XrVector3f* hmdPosition, XrVector3f* vsPosition)
{
	XrVector3f virtualScreenToHmd;
	XrVector3f_Sub(&virtualScreenToHmd, vsPosition, hmdPosition);
	const float distance = XrVector3f_Length(&virtualScreenToHmd);

	const float delta = (distance > targetDistance ? (distance - targetDistance) : (targetDistance - distance));
	if (delta > 0.001f)
	{
		XrVector3f scaledVirtualScreenToHmd;
		XrVector3f_Normalize(&virtualScreenToHmd);
		XrVector3f_Scale(&scaledVirtualScreenToHmd, &virtualScreenToHmd, targetDistance);
		XrVector3f_Add(vsPosition, hmdPosition, &scaledVirtualScreenToHmd);
	}
}


/*
==================
GetCurrentVirtualScreenPositionAndRotation

Anchor ladder. Fixed mode (vr_virtualScreenMode 0): anchor once per reset.
Follow mode (1): hysteresis re-targeting - settle when the in-front point is
within 0.1 m of the target, track once it drifts past 1.5 m, teleport past
3.0 m, drift at 0.01/frame with the head distance held constant.
==================
*/
static void GetCurrentVirtualScreenPositionAndRotation(XrPosef* headPose,
	XrVector3f* translation, XrQuaternionf* rotation)
{
	XrVector3f positionInFront = GetPositionInFront(headPose, targetDistance);

	if (!positionsInitialized)
	{
		currentPosition = positionInFront;
		targetPosition = positionInFront;
		currentRotation = YawFacingQuaternion(&currentPosition, &headPose->position);
		positionsInitialized = 1;
	}
	else if (vr_virtualScreenMode && vr_virtualScreenMode->integer == 1)
	{
		XrVector3f lastToCurrentVec;
		XrVector3f_Sub(&lastToCurrentVec, &targetPosition, &positionInFront);

		// The hysteresis was tuned at a 2.5 m anchor distance; scale the meter
		// thresholds with targetDistance so the angular feel stays the same
		// (re-target at ~35 deg of head turn, settle at ~2.3 deg) instead of
		// tightening as the distance grows
		const float settleDist   = 0.04f * targetDistance;
		const float retargetDist = 0.60f * targetDistance;
		const float teleportDist = 1.20f * targetDistance;

		const float updateTargetDist = XrVector3f_Length(&lastToCurrentVec);
		if (updateTargetDist < settleDist)
		{
			updateTarget = 0;
		}
		else if (updateTargetDist > retargetDist || updateTarget)
		{
			targetPosition = positionInFront;
			updateTarget = 1;

			if (updateTargetDist > teleportDist)
			{
				// Too far - teleport; we probably just started or switched into the virtual screen
				currentPosition = targetPosition;
			}
		}

		XrVector3f lerped;
		XrVector3f_Lerp(&lerped, &currentPosition, &targetPosition, 0.01f);
		EnsureNewPositionInExpectedDistance(&headPose->position, &lerped);
		currentPosition = lerped;
		currentRotation = YawFacingQuaternion(&currentPosition, &headPose->position);
	}

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
	centeredHead.position.x = (left->position.x + right->position.x) * 0.5f;
	centeredHead.position.y = (left->position.y + right->position.y) * 0.5f;
	centeredHead.position.z = (left->position.z + right->position.z) * 0.5f;
	centeredHead.orientation = left->orientation;

	// Head pose as a rigid transform and its inverse (the mono view)
	XrVector3f unitScale = { 1.0f, 1.0f, 1.0f };
	XrMatrix4x4f headPoseMat, headView;
	XrMatrix4x4f_CreateTranslationRotationScale(&headPoseMat, &centeredHead.position, &centeredHead.orientation, &unitScale);
	XrMatrix4x4f_InvertRigidBody(&headView, &headPoseMat);

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

		_VR_GetVirtualScreenModelMatrix(&screenModel, &centeredHead);
		_VR_GetVirtualScreenFloorModelMatrix(&floorModel);

		XrMatrix4x4f_Multiply(&mv, &headView, &screenModel);
		memcpy(screenModelView, mv.m, sizeof(mv.m));

		XrMatrix4x4f_Multiply(&mv, &headView, &floorModel);
		memcpy(floorModelView, mv.m, sizeof(mv.m));
	}

	return qtrue;
}


float VR_VirtualScreen_GetCurrentYaw(void)
{
	vec3_t rotation = { 0, 0, 0 };
	vec3_t yawPitchRoll = { 0, 0, 0 };
	QuatToYawPitchRoll(lastKnownVirtualScreenOrientation, rotation, yawPitchRoll);
	return yawPitchRoll[YAW];
}
