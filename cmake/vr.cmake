include_guard(GLOBAL)

# Find OpenXR - required for all VR code
find_package(OpenXR CONFIG REQUIRED)
list(APPEND VR_LIBRARIES OpenXR::openxr_loader OpenXR::headers)

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
# These are NOT added to VR_SOURCES - they are used only by renderer_vk.cmake
# via VR_VK_SOURCES variable
set(VR_VK_SOURCES
    ${SOURCE_DIR}/vrvk/vr_vk.c
    ${SOURCE_DIR}/vrvk/vr_vk_debug.c
    ${SOURCE_DIR}/vrvk/vr_vk_renderer.c
    ${SOURCE_DIR}/vrvk/vr_vk_session.c
    ${SOURCE_DIR}/vrvk/vr_vk_swapchains.c
    ${SOURCE_DIR}/vrvk/vr_vk_virtual_screen.c
)

# VR_SOURCES is used by the client executable
# It should contain vrcommon (always) + vrgl2 (for OpenGL clients)
# Vulkan-specific VR code (vrvk) is only linked into renderer_vulkan.dll
set(VR_SOURCES ${VR_COMMON_SOURCES})

# Add vrcommon to include directories (always needed)
list(APPEND VR_INCLUDE_DIRS ${SOURCE_DIR}/vrcommon)

if(BUILD_RENDERER_GL2)
    # OpenGL build gets vrgl2 sources for the client
    list(APPEND VR_SOURCES ${VR_GL2_SOURCES})
    find_package(OpenGL REQUIRED)
    list(APPEND VR_LIBRARIES ${OPENGL_LIBRARIES})
    list(APPEND VR_INCLUDE_DIRS ${SOURCE_DIR}/vrgl2)
endif()

# VR_SOURCES_VK combines vrcommon + vrvk for Vulkan clients
if(BUILD_RENDERER_VK)
    set(VR_SOURCES_VK ${VR_COMMON_SOURCES} ${VR_VK_SOURCES})
    list(APPEND VR_VK_INCLUDE_DIRS ${SOURCE_DIR}/vrcommon ${SOURCE_DIR}/vrvk)
endif()

list(APPEND RENDERER_INCLUDE_DIRS ${VR_INCLUDE_DIRS})
