# renderervk - Vulkan renderer for Q3VR
# Based on Quake3e's Vulkan renderer, adapted for OpenXR integration

if(NOT BUILD_CLIENT OR NOT BUILD_RENDERER_VK)
    return()
endif()

include(utils/set_output_dirs)
include(renderer_common)
include(compile_shaders)

set(RENDERER_VK_SOURCES
    ${SOURCE_DIR}/renderervk/tr_animation.c
    ${SOURCE_DIR}/renderervk/tr_backend.c
    ${SOURCE_DIR}/renderervk/tr_bsp.c
    ${SOURCE_DIR}/renderervk/tr_cmds.c
    ${SOURCE_DIR}/renderervk/tr_curve.c
    ${SOURCE_DIR}/renderervk/tr_image.c
    ${SOURCE_DIR}/renderervk/tr_init.c
    ${SOURCE_DIR}/renderervk/tr_light.c
    ${SOURCE_DIR}/renderervk/tr_main.c
    ${SOURCE_DIR}/renderervk/tr_marks.c
    ${SOURCE_DIR}/renderervk/tr_decals.c
    ${SOURCE_DIR}/renderervk/tr_mesh.c
    ${SOURCE_DIR}/renderervk/tr_model.c
    ${SOURCE_DIR}/renderervk/tr_model_iqm.c
    ${SOURCE_DIR}/renderervk/tr_scene.c
    ${SOURCE_DIR}/renderervk/tr_shade.c
    ${SOURCE_DIR}/renderervk/tr_shade_calc.c
    ${SOURCE_DIR}/renderervk/tr_shader.c
    ${SOURCE_DIR}/renderervk/tr_shadows.c
    ${SOURCE_DIR}/renderervk/tr_sky.c
    ${SOURCE_DIR}/renderervk/tr_surface.c
    ${SOURCE_DIR}/renderervk/tr_world.c
    ${SOURCE_DIR}/renderervk/vk.c
    ${SOURCE_DIR}/renderervk/vk_flares.c
    ${SOURCE_DIR}/renderervk/vk_vbo.c
)

# VR sources for Vulkan renderer
# VR_SOURCES_VK is defined in vr.cmake (vrcommon + vrvk)
include(vr)

set(RENDERER_VK_BASENAME renderer_vulkan)
set(RENDERER_VK_BINARY ${RENDERER_VK_BASENAME})

# Find Vulkan
find_package(Vulkan REQUIRED)

list(APPEND RENDERER_VK_BINARY_SOURCES
    ${RENDERER_COMMON_SOURCES}
    ${RENDERER_VK_SOURCES}
    ${RENDERER_LIBRARY_SOURCES})

list(APPEND RENDERER_VK_DEFINITIONS USE_VULKAN)
list(APPEND RENDERER_VK_LIBRARIES Vulkan::Vulkan)

if(USE_RENDERER_DLOPEN)
    list(APPEND RENDERER_VK_BINARY_SOURCES ${DYNAMIC_RENDERER_SOURCES})

    add_library(${RENDERER_VK_BINARY} SHARED ${RENDERER_VK_BINARY_SOURCES})

    target_link_libraries(      ${RENDERER_VK_BINARY} PRIVATE ${RENDERER_LIBRARIES} ${RENDERER_VK_LIBRARIES})
    target_include_directories( ${RENDERER_VK_BINARY} PRIVATE ${RENDERER_INCLUDE_DIRS})
    target_compile_definitions( ${RENDERER_VK_BINARY} PRIVATE ${RENDERER_DEFINITIONS} ${RENDERER_VK_DEFINITIONS})
    target_compile_options(     ${RENDERER_VK_BINARY} PRIVATE ${RENDERER_COMPILE_OPTIONS})
    target_link_options(        ${RENDERER_VK_BINARY} PRIVATE ${RENDERER_LINK_OPTIONS})

    # No "lib" prefix so the DLL/.so name matches cl_main.c's "renderer_%s".
    set_target_properties(${RENDERER_VK_BINARY} PROPERTIES PREFIX "")

    set_output_dirs(${RENDERER_VK_BINARY})

    # Ensure shaders are compiled before building renderer
    add_dependencies(${RENDERER_VK_BINARY} compile_shaders)
endif()
