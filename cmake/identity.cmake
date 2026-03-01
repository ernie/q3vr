set(PROJECT_NAME q3vr)
set(PROJECT_VERSION 1.0.0)

# Override version from CI tag (GITHUB_REF_NAME) or git tag
if(DEFINED ENV{GITHUB_REF_NAME} AND "$ENV{GITHUB_REF_NAME}" MATCHES "^v([0-9]+\\.[0-9]+(\\.[0-9]+)?)")
    set(PROJECT_VERSION "${CMAKE_MATCH_1}")
elseif(EXISTS "${CMAKE_SOURCE_DIR}/.git")
    execute_process(
        COMMAND git describe --tags --abbrev=0
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        OUTPUT_VARIABLE GIT_TAG
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
        RESULT_VARIABLE GIT_TAG_RESULT)

    if(GIT_TAG_RESULT EQUAL 0 AND GIT_TAG MATCHES "^v?([0-9]+\\.[0-9]+(\\.[0-9]+)?)")
        set(PROJECT_VERSION "${CMAKE_MATCH_1}")
    endif()
endif()

set(SERVER_NAME q3vr-ded)
set(CLIENT_NAME q3vr)

set(BASEGAME baseq3)

set(CGAME_MODULE cgame)
set(GAME_MODULE qagame)
set(UI_MODULE ui)

set(WINDOWS_ICON_PATH ${CMAKE_SOURCE_DIR}/misc/quake3.ico)

set(MACOS_ICON_PATH ${CMAKE_SOURCE_DIR}/misc/quake3_flat.icns)
set(MACOS_BUNDLE_ID org.ioquake.${CLIENT_NAME})

set(COPYRIGHT "QUAKE III ARENA Copyright © 1999-2000 id Software, Inc. All rights reserved.")

set(CONTACT_EMAIL "info@ioquake.org")
set(PROTOCOL_HANDLER_SCHEME quake3)
