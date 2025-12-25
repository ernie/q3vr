#ifndef __VR_DEBUG
#define __VR_DEBUG

#include "vr_types.h"

//
// XR Debug
//
void VR_ListAPILayers(void);
void VR_ListExtensions(void);

const char* GetXRErrorString(XrResult result);

void VR_CreateDebugUtilsMessenger(XrInstance instance, XrDebugUtilsMessengerEXT* messenger);
void VR_DestroyDebugUtilsMessenger(XrInstance instance, XrDebugUtilsMessengerEXT* messenger);

#endif
