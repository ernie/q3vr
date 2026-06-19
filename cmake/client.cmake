if(NOT BUILD_CLIENT)
    return()
endif()

include(utils/add_git_dependency)
include(utils/set_output_dirs)
include(shared_sources)

include(renderer_common)

include(vr)

set(CLIENT_SOURCES
    ${SOURCE_DIR}/client/cl_cgame.c
    ${SOURCE_DIR}/client/cl_cin.c
    ${SOURCE_DIR}/client/cl_console.c
    ${SOURCE_DIR}/client/cl_input.c
    ${SOURCE_DIR}/client/cl_keyboard.c
    ${SOURCE_DIR}/client/cl_keys.c
    ${SOURCE_DIR}/client/cl_main.c
    ${SOURCE_DIR}/client/cl_net_chan.c
    ${SOURCE_DIR}/client/cl_parse.c
    ${SOURCE_DIR}/client/cl_scrn.c
    ${SOURCE_DIR}/client/cl_ui.c
    ${SOURCE_DIR}/client/cl_voip.c
    ${SOURCE_DIR}/client/cl_avi.c
    ${SOURCE_DIR}/client/cl_trinity.c
    ${SOURCE_DIR}/client/cl_trinity_rconset.c
    ${SOURCE_DIR}/client/cl_tv.c
    ${SOURCE_DIR}/qcommon/autoupdate.c
    ${SOURCE_DIR}/client/libmumblelink.c
    ${SOURCE_DIR}/client/snd_altivec.c
    ${SOURCE_DIR}/client/snd_adpcm.c
    ${SOURCE_DIR}/client/snd_dma.c
    ${SOURCE_DIR}/client/snd_mem.c
    ${SOURCE_DIR}/client/snd_mix.c
    ${SOURCE_DIR}/client/snd_wavelet.c
    ${SOURCE_DIR}/client/snd_main.c
    ${SOURCE_DIR}/client/snd_codec.c
    ${SOURCE_DIR}/client/snd_codec_wav.c
    ${SOURCE_DIR}/client/snd_codec_ogg.c
    ${SOURCE_DIR}/client/snd_codec_opus.c
    ${SOURCE_DIR}/client/qal.c
    ${SOURCE_DIR}/client/snd_openal.c
    ${SOURCE_DIR}/sdl/sdl_input.c
    ${SOURCE_DIR}/sdl/sdl_snd.c
    ${CLIENT_PLATFORM_SOURCES}
)

# Discord Rich Presence talks to a local Discord client over IPC; there is no
# such client in a browser, so it is excluded from the Emscripten/web build.
# (cl_discord.h supplies no-op stubs there, so call sites stay unconditional.)
if(NOT EMSCRIPTEN)
    list(APPEND CLIENT_SOURCES
        ${SOURCE_DIR}/client/cl_discord_proto.c
        ${SOURCE_DIR}/client/cl_discord.c
    )
endif()

add_git_dependency(${SOURCE_DIR}/client/cl_console.c)

# CLIENT_DEFINITIONS is populated by library cmake files (opus.cmake, etc.)
# Add our core definitions here
list(APPEND CLIENT_DEFINITIONS Q3VR_VERSION_MAJOR=${PROJECT_VERSION_MAJOR})
if (PROJECT_VERSION_MINOR STREQUAL "")
    list(APPEND CLIENT_DEFINITIONS Q3VR_VERSION_MINOR=0)
else()
    list(APPEND CLIENT_DEFINITIONS Q3VR_VERSION_MINOR=${PROJECT_VERSION_MINOR})
endif()
if (PROJECT_VERSION_PATCH STREQUAL "")
    list(APPEND CLIENT_DEFINITIONS Q3VR_VERSION_PATCH=0)
else()
    list(APPEND CLIENT_DEFINITIONS Q3VR_VERSION_PATCH=${PROJECT_VERSION_PATCH})
endif()

list(APPEND CLIENT_DEFINITIONS Q3VR_VERSION="${Q3VR_VERSION_STRING}")

list(APPEND CLIENT_DEFINITIONS BOTLIB)

if(BUILD_STANDALONE)
    list(APPEND CLIENT_DEFINITIONS STANDALONE)
endif()

if(USE_RENDERER_DLOPEN)
    list(APPEND CLIENT_DEFINITIONS USE_RENDERER_DLOPEN)
endif()

if(USE_HTTP)
    list(APPEND CLIENT_DEFINITIONS USE_HTTP)
endif()

if(USE_VOIP)
    list(APPEND CLIENT_DEFINITIONS USE_VOIP)
endif()

if(USE_MUMBLE)
    list(APPEND CLIENT_DEFINITIONS USE_MUMBLE)
    list(APPEND CLIENT_LIBRARY_SOURCES ${SOURCE_DIR}/client/libmumblelink.c)
endif()

if(USE_DEBUG_STACKTRACE)
    list(APPEND CLIENT_DEFINITIONS USE_DEBUG_STACKTRACE)
endif()

list(APPEND CLIENT_INCLUDE_DIRS ${SOURCE_DIR}/libzstd)

set(CLIENT_BINARY_SOURCES_COMMON
    ${SERVER_SOURCES}
    ${CLIENT_SOURCES}
    ${COMMON_SOURCES}
    ${ZSTD_SOURCES}
    ${BOTLIB_SOURCES}
    ${SYSTEM_SOURCES}
    ${ASM_SOURCES}
    ${CLIENT_ASM_SOURCES}
    ${CLIENT_LIBRARY_SOURCES})

# Function to create a client executable with a specific renderer
function(add_client_executable TARGET_NAME RENDERER_TYPE VR_SOURCES_LIST)
    add_executable(${TARGET_NAME} ${CLIENT_EXECUTABLE_OPTIONS}
        ${CLIENT_BINARY_SOURCES_COMMON}
        ${VR_SOURCES_LIST})

    target_include_directories(${TARGET_NAME} PRIVATE ${CLIENT_INCLUDE_DIRS})
    target_compile_definitions(${TARGET_NAME} PRIVATE ${CLIENT_DEFINITIONS})
    target_compile_options(${TARGET_NAME} PRIVATE ${CLIENT_COMPILE_OPTIONS})
    target_link_libraries(${TARGET_NAME} PRIVATE ${COMMON_LIBRARIES} ${CLIENT_LIBRARIES} ${VR_LIBRARIES})
    target_link_options(${TARGET_NAME} PRIVATE ${CLIENT_LINK_OPTIONS})

    set_output_dirs(${TARGET_NAME})

    if(NOT USE_RENDERER_DLOPEN)
        if(RENDERER_TYPE STREQUAL "VK")
            target_sources(${TARGET_NAME} PRIVATE ${RENDERER_VK_BINARY_SOURCES})
            target_include_directories(${TARGET_NAME} PRIVATE ${RENDERER_INCLUDE_DIRS} ${VR_VK_INCLUDE_DIRS})
            target_compile_definitions(${TARGET_NAME} PRIVATE ${RENDERER_DEFINITIONS} ${RENDERER_VK_DEFINITIONS})
            target_compile_options(${TARGET_NAME} PRIVATE ${RENDERER_COMPILE_OPTIONS})
            target_link_libraries(${TARGET_NAME} PRIVATE ${RENDERER_LIBRARIES} ${RENDERER_VK_LIBRARIES})
            add_dependencies(${TARGET_NAME} compile_shaders)
        elseif(RENDERER_TYPE STREQUAL "GL2")
            target_sources(${TARGET_NAME} PRIVATE ${RENDERER_GL2_BINARY_SOURCES})
            target_include_directories(${TARGET_NAME} PRIVATE ${RENDERER_INCLUDE_DIRS} ${VR_INCLUDE_DIRS})
            target_compile_definitions(${TARGET_NAME} PRIVATE ${RENDERER_DEFINITIONS})
            target_compile_options(${TARGET_NAME} PRIVATE ${RENDERER_COMPILE_OPTIONS})
            target_link_libraries(${TARGET_NAME} PRIVATE ${RENDERER_LIBRARIES})
        endif()
    endif()

    foreach(LIBRARY IN LISTS CLIENT_DEPLOY_LIBRARIES)
        add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
                ${LIBRARY}
                $<TARGET_FILE_DIR:${TARGET_NAME}>)

        install(FILES ${LIBRARY} DESTINATION
            $<PATH:RELATIVE_PATH,$<TARGET_FILE_DIR:${TARGET_NAME}>,${CMAKE_BINARY_DIR}/$<CONFIG>>
            COMPONENT game_engine)
    endforeach()
endfunction()

# Build Vulkan renderer executable (trinityvr.exe)
if(BUILD_RENDERER_VK)
    # Use Vulkan-specific VR sources (vrcommon + vrvk)
    add_client_executable(${CLIENT_NAME} "VK" "${VR_SOURCES_VK}")
    set(PRIMARY_CLIENT ${CLIENT_NAME})
endif()

# Build OpenGL renderer executable (gltrinityvr.exe)
if(BUILD_RENDERER_GL2)
    # Use OpenGL-specific VR sources (vrcommon + vrgl2)
    add_client_executable(gl${CLIENT_NAME} "GL2" "${VR_SOURCES}")
    if(NOT DEFINED PRIMARY_CLIENT)
        set(PRIMARY_CLIENT gl${CLIENT_NAME})
    endif()
endif()

# Build pakQ3VR.pk3 from source (always regenerate to catch new files)
add_custom_target(pakQ3VR ALL
    COMMAND ${CMAKE_COMMAND} -E remove -f "${CMAKE_SOURCE_DIR}/assets/pakQ3VR.pk3"
    COMMAND ${CMAKE_COMMAND} -E tar cf
        ${CMAKE_SOURCE_DIR}/assets/pakQ3VR.pk3 --format=zip
        .
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/assets/pakQ3VR"
    COMMENT "Building pakQ3VR.pk3 from source"
)

# Copy assets to output dir (attach to primary client only)
if(DEFINED PRIMARY_CLIENT)
    add_custom_command(TARGET ${PRIMARY_CLIENT} POST_BUILD
        # Copy pakQ3VR.pk3 to both baseq3 and missionpack
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/assets/pakQ3VR.pk3"
        "$<TARGET_FILE_DIR:${PRIMARY_CLIENT}>/baseq3/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/assets/pakQ3VR.pk3"
        "$<TARGET_FILE_DIR:${PRIMARY_CLIENT}>/missionpack/"
        # Copy point release files
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/assets/third_party/point_release_v1.32"
        "$<TARGET_FILE_DIR:${PRIMARY_CLIENT}>/baseq3/"
        # Copy demo files
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/assets/third_party/demo"
        "$<TARGET_FILE_DIR:${PRIMARY_CLIENT}>/baseq3/"
        # Copy license files
        COMMAND ${CMAKE_COMMAND} -E make_directory
        "$<TARGET_FILE_DIR:${PRIMARY_CLIENT}>/licenses/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/THIRDPARTY.md"
        "$<TARGET_FILE_DIR:${PRIMARY_CLIENT}>/licenses/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/code/thirdparty/openal-soft-1.25.1/COPYING"
        "$<TARGET_FILE_DIR:${PRIMARY_CLIENT}>/licenses/COPYING.openal-soft"
    )
    # pakQ3VR is built unconditionally via `cmake -E tar`, so the POST_BUILD
    # copy above must always wait for it. (Previously guarded by ZIP_EXECUTABLE
    # back when a separate `zip` binary was required.)
    add_dependencies(${PRIMARY_CLIENT} pakQ3VR)
endif()

# Install targets
install(FILES "${CMAKE_SOURCE_DIR}/assets/pakQ3VR.pk3" DESTINATION
    $<PATH:RELATIVE_PATH,$<TARGET_FILE_DIR:${PRIMARY_CLIENT}>/baseq3/,${CMAKE_BINARY_DIR}/$<CONFIG>>
    COMPONENT game_engine)
install(FILES "${CMAKE_SOURCE_DIR}/assets/pakQ3VR.pk3" DESTINATION
    $<PATH:RELATIVE_PATH,$<TARGET_FILE_DIR:${PRIMARY_CLIENT}>/missionpack/,${CMAKE_BINARY_DIR}/$<CONFIG>>
    COMPONENT game_engine)
install(
    DIRECTORY "${CMAKE_SOURCE_DIR}/assets/third_party/point_release_v1.32/" DESTINATION
    $<PATH:RELATIVE_PATH,$<TARGET_FILE_DIR:${PRIMARY_CLIENT}>/baseq3/,${CMAKE_BINARY_DIR}/$<CONFIG>>
    COMPONENT point_release_patch)
install(
    DIRECTORY "${CMAKE_SOURCE_DIR}/assets/third_party/demo/" DESTINATION
    $<PATH:RELATIVE_PATH,$<TARGET_FILE_DIR:${PRIMARY_CLIENT}>/baseq3/,${CMAKE_BINARY_DIR}/$<CONFIG>>
    COMPONENT q3a_demo)
if(WIN32)
    install(FILES "${CMAKE_SOURCE_DIR}/misc/update-trinity.bat" DESTINATION
        $<PATH:RELATIVE_PATH,$<TARGET_FILE_DIR:${PRIMARY_CLIENT}>/,${CMAKE_BINARY_DIR}/$<CONFIG>>
        COMPONENT game_engine)
else()
    install(PROGRAMS "${CMAKE_SOURCE_DIR}/misc/update-trinity.sh" DESTINATION
        $<PATH:RELATIVE_PATH,$<TARGET_FILE_DIR:${PRIMARY_CLIENT}>/,${CMAKE_BINARY_DIR}/$<CONFIG>>
        COMPONENT game_engine)
endif()
