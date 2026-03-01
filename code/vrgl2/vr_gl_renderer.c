#include "../vrcommon/vr_renderer.h"

#include <math.h>

#include "../client/client.h"

#include "../vrcommon/common/xr_linear.h"
#include "../vrcommon/vr_base.h"
#include "../vrcommon/vr_clientinfo.h"
#include "../vrcommon/vr_events.h"
#include "../vrcommon/vr_gameplay.h"
#include "../vrcommon/vr_input.h"
#include "../vrcommon/vr_macros.h"
#include "../vrcommon/vr_math.h"
#include "../vrcommon/vr_render_loop.h"
#include "../vrcommon/vr_spaces.h"
#include "../vrcommon/vr_types.h"

// OpenGL-specific headers
#include "vr_gl_swapchains.h"
#include "vr_gl_virtual_screen.h"
#include "vr_gl_debug.h"

extern vr_clientinfo_t vr;
extern cvar_t *vr_heightAdjust;
extern cvar_t *vr_refreshrate;
extern cvar_t *vr_desktopContentType;

const float hudScale = M_PI * 15.0f / 180.0f;

XrBool32 stageSupported = XR_FALSE;
XrTime lastPredictedDisplayTime = 0;
qboolean frameStarted = qfalse;
qboolean needRecenter = qtrue;

// Data per-frame data held between BeginFrame and EndFrame
XrFovf fov = { 0 };
XrView views[2];
uint32_t viewCount = 2;
uint32_t swapchainColorIndex = 0;

void VR_Renderer_BeginFrame(VR_Engine* engine, XrBool32 needsRecenter);
void VR_Renderer_EndFrame(VR_Engine* engine);
void VR_Recenter(VR_Engine* engine, XrTime predictedDisplayTime);
void VR_ClearFrameBuffer( int width, int height);
void VR_UpdatePerFrameState( void );
void VR_DrawVirtualScreen(VR_SwapchainInfos* swapchains, uint32_t swapchainImageIndex, XrFovf frameFov, XrView* frameViews, uint32_t frameViewCount);
XrDesktopViewConfiguration VR_GetDesktopViewConfiguration( void );

void VR_GetResolution(VR_Engine* engine, int *pWidth, int *pHeight)
{
	VR_GetSupersampledResolution(engine->appState.Instance, engine->appState.SystemId, pWidth, pHeight);
}

void VR_InitRenderer( VR_Engine* engine )
{
	VR_GL_RegisterDebugLogSinkIfEnabled();

	// Get the supported display refresh rates for the system.
	{
		PFN_xrGetDisplayRefreshRateFB xrGetDisplayRefreshRateFB;
		XR_CHECK(
			xrGetInstanceProcAddr(engine->appState.Instance, "xrGetDisplayRefreshRateFB", (PFN_xrVoidFunction*)(&xrGetDisplayRefreshRateFB)),
			"failed to get xrGetDisplayRefreshRateFB func proc");

		engine->appState.Renderer.RefreshRate = 0.0f;
		XR_CHECK(
				xrGetDisplayRefreshRateFB(engine->appState.Session, &engine->appState.Renderer.RefreshRate),
				"failed to get current display refresh rate");
		printf("Current System Display Refresh Rate: %f\n", engine->appState.Renderer.RefreshRate);

		Cvar_SetValue("vr_refreshrate", engine->appState.Renderer.RefreshRate);
	}

	stageSupported = VR_IsStageSpaceSupported(engine->appState.Session);

	if (engine->appState.CurrentSpace == XR_NULL_HANDLE)
	{
		XrTime nullTime = 0; // won't be used anyway
		VR_Recenter(engine, nullTime);
	}

	// Create VIEW reference space for head-locked quad layers
	{
		XrReferenceSpaceCreateInfo viewSpaceCI = {};
		viewSpaceCI.type = XR_TYPE_REFERENCE_SPACE_CREATE_INFO;
		viewSpaceCI.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW;
		viewSpaceCI.poseInReferenceSpace.orientation.w = 1.0f;  // Identity
		XR_CHECK(
			xrCreateReferenceSpace(engine->appState.Session, &viewSpaceCI, &engine->appState.ViewSpace),
			"Failed to create VIEW reference space for quad layer");
		printf("Created VIEW reference space for head-locked quad layers\n");
	}

	engine->appState.Renderer.Swapchains = VR_CreateSwapchains(
		engine->appState.Instance,
		engine->appState.SystemId,
		engine->appState.Session);

	VR_VirtualScreen_Init();
	VR_VirtualScreen_ResetPosition();
}

void VR_DestroyRenderer( VR_Engine* engine )
{
	VR_VirtualScreen_Destroy();
	VR_DestroySwapchains(&engine->appState.Renderer.Swapchains);  // Takes pointer to pointer, sets to NULL

	// Destroy VIEW reference space
	if (engine->appState.ViewSpace != XR_NULL_HANDLE)
	{
		xrDestroySpace(engine->appState.ViewSpace);
		engine->appState.ViewSpace = XR_NULL_HANDLE;
	}
}

void VR_ProcessFrame( VR_Engine* engine )
{
	VR_UpdatePerFrameState();

	const XrBool32 needsRecenter = VR_ProcessXrEvents(&engine->appState);
	if (engine->appState.SessionActive == VR_FALSE)
	{
		// If we haven't called Com_Frame() then let's at least process input
		// (specifically SDL events) so that app won't appear as stuck/deadlocked
		IN_Frame();
		return;
	}

	VR_Renderer_BeginFrame(engine, needsRecenter);
	Com_Frame();
	VR_Renderer_EndFrame(engine);

	if (needRecenter)
	{
		VR_Recenter(engine, lastPredictedDisplayTime);
		needRecenter = qfalse;
	}
}

void VR_Renderer_RestoreState(VR_Engine* engine)
{
	if (!frameStarted)
	{
		// Frame hasn't started, no need to restore anything here
		return;
	}

	VR_UpdatePerFrameState();

	// If we need to re-start frame until `Com_Frame()` call, we need session to
	// be active to proceed
	XrBool32 needsRecenter = XR_FALSE;
	while (!engine->appState.SessionActive)
	{
		needsRecenter |= VR_ProcessXrEvents(&engine->appState);
	}

	VR_Renderer_BeginFrame(engine, needsRecenter);
}

void VR_Renderer_BeginFrame(VR_Engine* engine, XrBool32 needsRecenter)
{
	frameStarted = qtrue;
	lastPredictedDisplayTime = VR_WaitFrame(engine->appState.Session).predictedDisplayTime;

	if (needsRecenter)
	{
		VR_Recenter(engine, lastPredictedDisplayTime);
	}

	VR_BeginFrame(engine->appState.Session);

	VR_LocateViews(
		engine->appState.Session,
		lastPredictedDisplayTime,
		engine->appState.CurrentSpace,
		views,
		&viewCount);

	// Update HMD position/views
	IN_VRUpdateHMD(views, viewCount, &fov);

	// SP intermission state tracking - must be set before rendering
	// so UI code sees the correct state for scaling/offsets
	qboolean isSPIntermission = VR_IsSPIntermission();
	if (isSPIntermission && !vr.sp_intermission_active)
	{
		// First frame of SP intermission - capture anchor position
		vr.sp_intermission_active = qtrue;
		// Store yaw for HUD positioning (in degrees)
		XrQuaternionf q = views[0].pose.orientation;
		float siny_cosp = 2.0f * (q.w * q.y + q.z * q.x);
		float cosy_cosp = 1.0f - 2.0f * (q.x * q.x + q.y * q.y);
		vr.sp_intermission_yaw = atan2f(siny_cosp, cosy_cosp) * 180.0f / (float)M_PI;
	}
	else if (!isSPIntermission && vr.sp_intermission_active)
	{
		// Exiting SP intermission - reset state
		vr.sp_intermission_active = qfalse;
	}

	// [Input] poll actions, update controller state, issue action commands
	IN_VRSyncActions(engine);
	IN_VRUpdateControllers(engine, lastPredictedDisplayTime);

	VR_SwapchainInfos* swapchains = engine->appState.Renderer.Swapchains;

	VR_Swapchains_Acquire(swapchains, &swapchainColorIndex);
	VR_Swapchains_BindFramebuffers(swapchains, swapchainColorIndex);
	VR_ClearFrameBuffer(swapchains->color.width, swapchains->color.height);

	// Set renderer params
	// Near plane must be in Quake units to match our view matrices
	// Default r_znear is 4 Quake units. With worldscale=32, that's 4/32 = 0.125 meters
	// Using too small a near plane (like 1.0) makes things appear too close
	float nearPlane = 4.0f;  // Match r_znear default in Quake units

	// OpenGL clip space convention for projection matrix
	const GraphicsAPI graphicsApi = GRAPHICS_OPENGL;

	XrMatrix4x4f vrMatrixMono, vrMatrixProjection;
	const XrFovf monoFov = { -hudScale, hudScale, hudScale, -hudScale };
	const XrFovf projectionFov =
	{
		fov.angleLeft / vr.weapon_zoomLevel,
		fov.angleRight / vr.weapon_zoomLevel,
		fov.angleUp / vr.weapon_zoomLevel,
		fov.angleDown / vr.weapon_zoomLevel,
	};
	XrMatrix4x4f_CreateProjectionFov(&vrMatrixMono, graphicsApi, monoFov, nearPlane, 0.0f);
	XrMatrix4x4f_CreateProjectionFov(&vrMatrixProjection, graphicsApi, projectionFov, nearPlane, 0.0f);

	// Create per-eye projection matrices from actual OpenXR FOVs
	// These asymmetric projections must be paired with matching per-eye view positions
	XrMatrix4x4f vrMatrixEye[2];
	for (int eye = 0; eye < 2 && eye < (int)viewCount; eye++)
	{
		XrFovf eyeFov = {
			views[eye].fov.angleLeft / vr.weapon_zoomLevel,
			views[eye].fov.angleRight / vr.weapon_zoomLevel,
			views[eye].fov.angleUp / vr.weapon_zoomLevel,
			views[eye].fov.angleDown / vr.weapon_zoomLevel,
		};
		XrMatrix4x4f_CreateProjectionFov(&vrMatrixEye[eye], graphicsApi, eyeFov, nearPlane, 0.0f);
	}

	// Compute combined stereo horizontal FOV for culling (encompasses both eyes)
	// Use leftmost angle from left eye, rightmost from right eye
	// Vertical FOV doesn't need combining since eyes are horizontally separated
	float combinedAngleLeft = views[0].fov.angleLeft / vr.weapon_zoomLevel;
	float combinedAngleRight = views[1].fov.angleRight / vr.weapon_zoomLevel;

	// Convert to degrees for Q3's FOV system
	float combinedFovX = (fabsf(combinedAngleLeft) + fabsf(combinedAngleRight)) * 180.0f / M_PI;

	// Calculate half-IPD in meters for frustum plane offset
	float halfIpdMeters = 0.0f;
	if (viewCount >= 2) {
		float dx = views[1].pose.position.x - views[0].pose.position.x;
		float dy = views[1].pose.position.y - views[0].pose.position.y;
		float dz = views[1].pose.position.z - views[0].pose.position.z;
		halfIpdMeters = sqrtf(dx*dx + dy*dy + dz*dz) * 0.5f;
	}

	re.SetVRHeadsetParms(vrMatrixProjection.m, vrMatrixMono.m, swapchains->framebuffers[swapchainColorIndex],
						 vrMatrixEye[0].m, vrMatrixEye[1].m, combinedFovX, halfIpdMeters);
}

void VR_Renderer_EndFrame(VR_Engine* engine)
{
	VR_SwapchainInfos* swapchains = engine->appState.Renderer.Swapchains;

	// Draw Virtual Screen if needed
	const int use_virtual_screen = VR_Gameplay_ShouldRenderInVirtualScreen();

	if (use_virtual_screen)
	{
		VR_DrawVirtualScreen(swapchains, swapchainColorIndex, fov, views, viewCount);
		if ( !vr.menuYawLocked ) {
			vr.menuYaw = VR_VirtualScreen_GetCurrentYaw();
		}
	}
	else
	{
		VR_VirtualScreen_ResetPosition();
		if ( !vr.menuYawLocked ) {
			vr.menuYaw = vr.hmdorientation[YAW];
		}
	}

	VR_Swapchains_Release(swapchains);

	VR_Swapchains_BindFramebuffers(NULL, 0);

	// Blit to main FBO (desktop window) - use virtual screen if active, otherwise eye view
	VR_Swapchains_BlitXRToMainFbo(swapchains, swapchainColorIndex, VR_GetDesktopViewConfiguration(), use_virtual_screen);

	VR_EndFrame(
		engine->appState.Session,
		swapchains,
		views,
		viewCount,
		fov,
		engine->appState.CurrentSpace,
		engine->appState.ViewSpace,
		lastPredictedDisplayTime);

	// Flip desktop window's buffer - use renderer export for abstraction
	re.SwapDesktopWindow();

	frameStarted = qfalse;
}

void VR_Recenter(VR_Engine* engine, XrTime predictedDisplayTime)
{
	// Calculate recenter reference
	XrReferenceSpaceCreateInfo spaceCreateInfo = {};
	spaceCreateInfo.type = XR_TYPE_REFERENCE_SPACE_CREATE_INFO;
	spaceCreateInfo.poseInReferenceSpace.orientation.w = 1.0f;
	if (engine->appState.CurrentSpace != XR_NULL_HANDLE)
	{
		vec3_t rotation = {0, 0, 0};
		XrSpaceLocation loc = {};
		loc.type = XR_TYPE_SPACE_LOCATION;
		XR_CHECK(
			xrLocateSpace(engine->appState.HeadSpace, engine->appState.CurrentSpace, predictedDisplayTime, &loc),
			"Failed to locate space");
		QuatToYawPitchRoll(loc.pose.orientation, rotation, vr.hmdorientation);

		vr.recenterYaw += DegreesToRadians(vr.hmdorientation[YAW]);
		spaceCreateInfo.poseInReferenceSpace.orientation.x = 0;
		spaceCreateInfo.poseInReferenceSpace.orientation.y = sin(vr.recenterYaw / 2);
		spaceCreateInfo.poseInReferenceSpace.orientation.z = 0;
		spaceCreateInfo.poseInReferenceSpace.orientation.w = cos(vr.recenterYaw / 2);
	}

	// Delete previous space instances
	if (engine->appState.StageSpace != XR_NULL_HANDLE)
	{
		XR_CHECK(
			xrDestroySpace(engine->appState.StageSpace),
			"Failed to destroy stage space");
	}
	if (engine->appState.FakeStageSpace != XR_NULL_HANDLE)
	{
		XR_CHECK(
			xrDestroySpace(engine->appState.FakeStageSpace),
			"Failed to destroy fake stage space");
	}

	// Create a default stage space to use if SPACE_TYPE_STAGE is not
	// supported, or calls to xrGetReferenceSpaceBoundsRect fail.
	spaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
	spaceCreateInfo.poseInReferenceSpace.position.y = -1.6750f;
	XR_CHECK(
		xrCreateReferenceSpace(engine->appState.Session, &spaceCreateInfo, &engine->appState.FakeStageSpace),
		"Failed to create reference space (fake stage)");
	printf("Created fake stage space from local space with offset\n");
	engine->appState.CurrentSpace = engine->appState.FakeStageSpace;

	if (stageSupported)
	{
		spaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_STAGE;
		spaceCreateInfo.poseInReferenceSpace.position.y = 0.0f;
		XR_CHECK(
			xrCreateReferenceSpace(engine->appState.Session, &spaceCreateInfo, &engine->appState.StageSpace),
			"Failed to create reference space (stage)");
		printf("Created stage space\n");
		engine->appState.CurrentSpace = engine->appState.StageSpace;
	}

	// Update menu orientation
	vr.menuYaw = 0;

	// Reset VirtualScreen's position
	VR_VirtualScreen_ResetPosition();
}

void VR_ClearFrameBuffer( int width, int height)
{
	// Delegate to renderer - avoids direct graphics API calls in VR layer
	qboolean isThirdPersonSpectator = Cvar_VariableIntegerValue("vr_thirdPersonSpectator") ? qtrue : qfalse;
	re.ClearVRFramebuffer(width, height, isThirdPersonSpectator);
}

void VR_UpdatePerFrameState( void )
{
	if (vr.weapon_zoomed)
	{
		vr.weapon_zoomLevel += 0.05f;
		if (vr.weapon_zoomLevel > 2.5f)
				vr.weapon_zoomLevel = 2.5f;
	}
	else
	{
		//Zoom back out quicker
		vr.weapon_zoomLevel -= 0.25f;
		if (vr.weapon_zoomLevel < 1.0f)
				vr.weapon_zoomLevel = 1.0f;
	}
}

void VR_DrawVirtualScreen(VR_SwapchainInfos* swapchains, uint32_t swapchainImageIndex, XrFovf frameFov, XrView* frameViews, uint32_t frameViewCount)
{
	// Copy current image to Virtual Screen's texture
	VR_Swapchains_BlitXRToVirtualScreen(swapchains, swapchainImageIndex);

	// Clear framebuffer before drawing virtual screen
	// Using re.ClearVRFramebuffer handles the viewport/scissor setup
	re.ClearVRFramebuffer(swapchains->color.width, swapchains->color.height, qfalse);

	VR_VirtualScreen_Draw(frameViews, frameViewCount, swapchains->color.virtualScreenImage);
}

XrDesktopViewConfiguration VR_GetDesktopViewConfiguration( void )
{
	if (vr_desktopContentType)
	{
		switch (vr_desktopContentType->integer)
		{
			case 0:
				return LEFT_EYE;
			case 1:
				return RIGHT_EYE;
			case 2:
				return BOTH_EYES;
		}
	}
	return LEFT_EYE;
}

qboolean VR_Renderer_SubmitLoadingFrame(VR_Engine* engine)
{
	// Only submit frames during loading states when a frame has been started
	if ((clc.state != CA_LOADING && clc.state != CA_PRIMED) || !frameStarted)
	{
		return qfalse;
	}

	// End the current VR frame (this will blit to virtual screen and submit to XR)
	VR_Renderer_EndFrame(engine);

	// Start a new VR frame for the next screen update
	VR_Renderer_BeginFrame(engine, XR_FALSE);

	return qtrue;
}
