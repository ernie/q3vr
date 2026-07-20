#ifndef __VR_CYLINDER_DRAW
#define __VR_CYLINDER_DRAW

#include <openxr/openxr.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>

#include "../vrcommon/vr_macros.h"
#include "../vrcommon/vr_graphics.h"
#include "../renderercommon/tr_common.h"   // qgl function-pointer declarations

void VRGL_VirtualScreen_Init(void);
void VRGL_VirtualScreen_Destroy(void);
void VR_VirtualScreen_Draw(XrView* views, uint32_t viewCount, GLuint virtualScreenImage);

#endif
