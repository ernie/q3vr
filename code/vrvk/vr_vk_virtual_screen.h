/*
 * vr_vk_virtual_screen.h - Vulkan virtual screen declarations
 *
 * Virtual screen is used for menus, console, and first-person follow mode.
 * The game renders to a texture which is then displayed as a curved 3D quad.
 *
 * Anchor state and pose math live in vrcommon/vr_virtual_screen.c; this
 * layer owns only the Vulkan-facing split-transform query.
 */

#ifndef VR_VK_VIRTUAL_SCREEN_H
#define VR_VK_VIRTUAL_SCREEN_H

#include "../qcommon/q_shared.h"
#include "../vrcommon/vr_types.h"
#include "../vrcommon/vr_graphics.h"

void VR_VirtualScreen_Init(void);
void VR_VirtualScreen_Destroy(void);

// Query function for renderer to pull virtual screen state (pull model)
// Returns qtrue if virtual screen should be rendered
// If active, fills in the split transform: shared per-eye eyeProj pair plus one mono model-view per mesh
qboolean VR_GetVirtualScreenMatrices(float eyeProj[2][16], float screenModelView[16], float floorModelView[16]);

#endif // VR_VK_VIRTUAL_SCREEN_H
