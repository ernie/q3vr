if(NOT USE_OPENAL)
    return()
endif()

if(NOT BUILD_CLIENT)
    return()
endif()

set(INTERNAL_OPENAL_DIR ${SOURCE_DIR}/thirdparty/openal-soft-1.25.1)

find_package(OpenAL QUIET)

if(NOT OpenAL_FOUND)
    set(OPENAL_DEFINITIONS USE_INTERNAL_OPENAL_HEADERS)
    set(OPENAL_INCLUDE_DIR ${INTERNAL_OPENAL_DIR}/include)
    set(OPENAL_LIBRARY openal)
endif()

list(APPEND CLIENT_DEFINITIONS ${OPENAL_DEFINITIONS} USE_OPENAL)
list(APPEND CLIENT_INCLUDE_DIRS ${OPENAL_INCLUDE_DIR})

if(USE_OPENAL_DLOPEN)
    list(APPEND CLIENT_DEFINITIONS USE_OPENAL_DLOPEN)

    # Bundle OpenAL Soft DLL with the build output
    if(WIN32)
        include(utils/arch)
        if(ARCH STREQUAL "x86_64")
            set(OPENAL_DLL ${SOURCE_DIR}/thirdparty/libs/win64/OpenAL64.dll)
        elseif(ARCH STREQUAL "x86")
            set(OPENAL_DLL ${SOURCE_DIR}/thirdparty/libs/win32/OpenAL32.dll)
        endif()
        if(DEFINED OPENAL_DLL AND EXISTS ${OPENAL_DLL})
            list(APPEND CLIENT_DEPLOY_LIBRARIES ${OPENAL_DLL})
        endif()
    endif()
else()
    find_package(Threads REQUIRED)
    list(APPEND CLIENT_LIBRARIES Threads::Threads ${OPENAL_LIBRARY})
endif()
