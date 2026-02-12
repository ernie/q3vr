if(NOT USE_HTTP)
    return()
endif()

if(NOT BUILD_CLIENT)
    return()
endif()

set(INTERNAL_CURL_DIR ${SOURCE_DIR}/thirdparty/curl-8.15.0)

if(WIN32)
    # Build curl from source as a static library on Windows.
    # This avoids pulling in libcurl.dll and zlib1.dll via vcpkg.
    include(FetchContent)

    set(BUILD_CURL_EXE OFF CACHE BOOL "" FORCE)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
    set(BUILD_LIBCURL_DOCS OFF CACHE BOOL "" FORCE)

    # TLS: use Windows-native Schannel
    set(CURL_USE_SCHANNEL ON CACHE BOOL "" FORCE)
    set(CURL_USE_OPENSSL OFF CACHE BOOL "" FORCE)
    set(CURL_USE_MBEDTLS OFF CACHE BOOL "" FORCE)
    set(CURL_USE_WOLFSSL OFF CACHE BOOL "" FORCE)
    set(CURL_WINDOWS_SSPI ON CACHE BOOL "" FORCE)

    # Disable zlib — the game engine has its own internal zlib for pk3 handling
    set(CURL_ZLIB OFF CACHE BOOL "" FORCE)

    # HTTP(S) only
    set(HTTP_ONLY ON CACHE BOOL "" FORCE)

    # Disable optional features we don't need
    set(CURL_DISABLE_LDAP ON CACHE BOOL "" FORCE)
    set(CURL_DISABLE_LDAPS ON CACHE BOOL "" FORCE)
    set(ENABLE_ARES OFF CACHE BOOL "" FORCE)
    set(CURL_BROTLI OFF CACHE BOOL "" FORCE)
    set(CURL_ZSTD OFF CACHE BOOL "" FORCE)
    set(CURL_USE_LIBPSL OFF CACHE BOOL "" FORCE)
    set(CURL_USE_LIBSSH2 OFF CACHE BOOL "" FORCE)
    set(USE_NGHTTP2 OFF CACHE BOOL "" FORCE)
    set(USE_LIBIDN2 OFF CACHE BOOL "" FORCE)
    set(USE_WIN32_IDN OFF CACHE BOOL "" FORCE)
    set(ENABLE_CURL_MANUAL OFF CACHE BOOL "" FORCE)

    FetchContent_Declare(
        curl_static
        GIT_REPOSITORY https://github.com/curl/curl.git
        GIT_TAG        curl-8_15_0
        GIT_SHALLOW    TRUE
        EXCLUDE_FROM_ALL
    )
    FetchContent_MakeAvailable(curl_static)

    list(APPEND CLIENT_LIBRARIES libcurl_static)
    list(APPEND CLIENT_DEFINITIONS CURL_STATICLIB)
    list(APPEND CLIENT_LIBRARIES wldap32 crypt32)
else()
    # Non-Windows: try system curl first, fall back to runtime loading
    find_package(CURL QUIET)

    if(CURL_FOUND)
        list(APPEND CLIENT_LIBRARIES CURL::libcurl)
    else()
        set(CURL_DEFINITIONS USE_CURL_DLOPEN USE_INTERNAL_CURL_HEADERS)
        set(CURL_INCLUDE_DIRS ${INTERNAL_CURL_DIR}/include)
        list(APPEND CLIENT_DEFINITIONS ${CURL_DEFINITIONS})
        list(APPEND CLIENT_INCLUDE_DIRS ${CURL_INCLUDE_DIRS})
    endif()
endif()
