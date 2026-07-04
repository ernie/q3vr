/*
 * vr_vk_virtual_screen.h - Vulkan virtual screen declarations
 *
 * Virtual screen is used for menus, console, and first-person follow mode.
 * The game renders to a texture which is then displayed as a curved 3D quad.
 *
 * Phase 8 - stub implementations provided here for Phase 4 linker compatibility.
 */

#ifndef VR_VK_VIRTUAL_SCREEN_H
#define VR_VK_VIRTUAL_SCREEN_H

#include "../qcommon/q_shared.h"
#include "../vrcommon/vr_types.h"

void VR_VirtualScreen_Init(void);
void VR_VirtualScreen_Destroy(void);
void VR_VirtualScreen_ResetPosition(void);
float VR_VirtualScreen_GetCurrentYaw(void);

// Query function for renderer to pull virtual screen state (pull model)
// Returns qtrue if virtual screen should be rendered
// If active, fills in precomputed per-eye MVP matrices for screen and floor
qboolean VR_GetVirtualScreenMatrices(float eyeProj[2][16], float screenModelView[16], float floorModelView[16]);

#endif // VR_VK_VIRTUAL_SCREEN_H
