/*
 * vr_virtual_screen.h - renderer-agnostic virtual-screen anchor queries
 *
 * VR_VirtualScreen_ResetPosition / VR_VirtualScreen_GetCurrentYaw are part
 * of the graphics-agnostic contract in vr_graphics.h; this header carries
 * only the XR-typed pose/matrix queries so vr_graphics.h stays free of
 * xr_linear types.
 */

#ifndef VR_VIRTUAL_SCREEN_H
#define VR_VIRTUAL_SCREEN_H

#include <openxr/openxr.h>

#include "common/xr_linear.h"

// Eye-midpoint position with the left eye's orientation - anchoring uses a
// single centered head pose so the screen is not offset toward one eye
void VR_VirtualScreen_ComputeCenteredHeadPose(XrPosef* out, const XrView* views, uint32_t viewCount);

// Screen model matrix: runs the anchor ladder (fixed or follow mode),
// applies aspect compensation, and records the orientation reported by
// VR_VirtualScreen_GetCurrentYaw
void VR_VirtualScreen_GetModelMatrix(XrMatrix4x4f* model, const XrPosef* centeredHead);

// Floor-grid model matrix, aligned with stage space via vr.recenterYaw
void VR_VirtualScreen_GetFloorModelMatrix(XrMatrix4x4f* model);

// View matrix from an eye pose: inverse of (translation * rotation)
void VR_VirtualScreen_GetViewMatrix(XrMatrix4x4f* result, const XrVector3f* translation, const XrQuaternionf* rotation);

#endif // VR_VIRTUAL_SCREEN_H
