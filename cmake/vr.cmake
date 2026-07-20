include_guard(GLOBAL)

# Find OpenXR - required for all VR code
find_package(OpenXR CONFIG REQUIRED)
list(APPEND VR_LIBRARIES OpenXR::openxr_loader OpenXR::headers)

# vr_types.h includes vulkan.h in every configuration (runtime backend dispatch)
find_package(Vulkan REQUIRED)
list(APPEND VR_INCLUDE_DIRS ${Vulkan_INCLUDE_DIRS})

# vrcommon - Renderer-agnostic VR sources (shared by all renderers)
set(VR_COMMON_SOURCES
    ${SOURCE_DIR}/vrcommon/vr_cvars.c
    ${SOURCE_DIR}/vrcommon/vr_debug.c
    ${SOURCE_DIR}/vrcommon/vr_events.c
    ${SOURCE_DIR}/vrcommon/vr_gameplay.c
    ${SOURCE_DIR}/vrcommon/vr_haptics.c
    ${SOURCE_DIR}/vrcommon/vr_bhaptics.c
    ${SOURCE_DIR}/vrcommon/vr_input.c
    ${SOURCE_DIR}/vrcommon/vr_math.c
    ${SOURCE_DIR}/vrcommon/vr_spaces.c
    ${SOURCE_DIR}/vrcommon/vr_swapchains.c
    ${SOURCE_DIR}/vrcommon/vr_base.c
    ${SOURCE_DIR}/vrcommon/vr_shared_sync.c
    ${SOURCE_DIR}/vrcommon/vr_instance.c
    ${SOURCE_DIR}/vrcommon/vr_render_loop.c
    ${SOURCE_DIR}/vrcommon/vr_session.c
    ${SOURCE_DIR}/vrcommon/vr_virtual_screen.c
    ${SOURCE_DIR}/vrcommon/vr_backend.c
)

# vrgl2 - OpenGL-specific VR sources
set(VR_GL2_SOURCES
    ${SOURCE_DIR}/vrgl2/vr_gl.c
    ${SOURCE_DIR}/vrgl2/vr_gl_debug.c
    ${SOURCE_DIR}/vrgl2/vr_gl_renderer.c
    ${SOURCE_DIR}/vrgl2/vr_gl_session.c
    ${SOURCE_DIR}/vrgl2/vr_gl_swapchains.c
    ${SOURCE_DIR}/vrgl2/vr_gl_virtual_screen.c
)

# vrvk - Vulkan-specific VR sources (XR_KHR_vulkan_enable2 integration)
set(VR_VK_SOURCES
    ${SOURCE_DIR}/vrvk/vr_vk.c
    ${SOURCE_DIR}/vrvk/vr_vk_debug.c
    ${SOURCE_DIR}/vrvk/vr_vk_renderer.c
    ${SOURCE_DIR}/vrvk/vr_vk_session.c
    ${SOURCE_DIR}/vrvk/vr_vk_swapchains.c
    ${SOURCE_DIR}/vrvk/vr_vk_virtual_screen.c
)

# The single client links every VR backend (vrcommon + vrgl2 + vrvk); the
# renderer DLL and matching backend are chosen at runtime via cl_renderer.
set(VR_SOURCES ${VR_COMMON_SOURCES} ${VR_GL2_SOURCES} ${VR_VK_SOURCES})

# The client links both graphics loaders: vrvk makes Vulkan calls (cl_main.c
# needs vkGetInstanceProcAddr) and vrgl2 makes raw gl* calls.
find_package(OpenGL REQUIRED)
list(APPEND VR_LIBRARIES Vulkan::Vulkan ${OPENGL_LIBRARIES})

list(APPEND VR_INCLUDE_DIRS
    ${SOURCE_DIR}/vrcommon
    ${SOURCE_DIR}/vrgl2
    ${SOURCE_DIR}/vrvk)

list(APPEND RENDERER_INCLUDE_DIRS ${VR_INCLUDE_DIRS})
