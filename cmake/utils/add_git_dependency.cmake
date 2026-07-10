include_guard(GLOBAL)

# Rebuild SOURCE_FILE when the git HEAD or current branch ref moves (version
# stamping). Handles .git as a directory (normal checkout) and as a
# "gitdir:" pointer file (worktree), where HEAD is private to the worktree
# but packed-refs and refs/ live in the common dir.
# Note: OBJECT_DEPENDS only drives rebuilds under Makefile/Ninja generators
# (e.g. Linux CI); the Visual Studio generator ignores the property, so MSVC
# builds keep a stale version stamp until the file recompiles for other
# reasons.
function(add_git_dependency SOURCE_FILE)
    set(GIT_DIR "${CMAKE_SOURCE_DIR}/.git")
    if(NOT EXISTS "${GIT_DIR}")
        return()
    endif()

    if(NOT IS_DIRECTORY "${GIT_DIR}")
        file(READ "${GIT_DIR}" GIT_POINTER)
        string(REGEX MATCH "gitdir: ([^\n]+)" HAVE_GITDIR "${GIT_POINTER}")
        if(NOT HAVE_GITDIR)
            return()
        endif()
        string(STRIP "${CMAKE_MATCH_1}" GIT_DIR)
        if(NOT IS_ABSOLUTE "${GIT_DIR}")
            get_filename_component(GIT_DIR "${CMAKE_SOURCE_DIR}/${GIT_DIR}" ABSOLUTE)
        endif()
        if(NOT EXISTS "${GIT_DIR}/HEAD")
            return()
        endif()
    endif()

    set(GIT_COMMON_DIR "${GIT_DIR}")
    if(EXISTS "${GIT_DIR}/commondir")
        file(READ "${GIT_DIR}/commondir" GIT_COMMON_PATH)
        string(STRIP "${GIT_COMMON_PATH}" GIT_COMMON_PATH)
        if(NOT IS_ABSOLUTE "${GIT_COMMON_PATH}")
            get_filename_component(GIT_COMMON_PATH "${GIT_DIR}/${GIT_COMMON_PATH}" ABSOLUTE)
        endif()
        set(GIT_COMMON_DIR "${GIT_COMMON_PATH}")
    endif()

    set(GIT_FILES)
    list(APPEND GIT_FILES "${GIT_DIR}/HEAD")
    list(APPEND GIT_FILES "${GIT_COMMON_DIR}/packed-refs")

    file(READ "${GIT_DIR}/HEAD" GIT_HEAD)
    string(REGEX MATCH "^ref: (.+)$" HAVE_REF "${GIT_HEAD}")
    if(HAVE_REF)
        set(GIT_REF_PATH "${CMAKE_MATCH_1}")
        string(STRIP "${GIT_REF_PATH}" GIT_REF_PATH)
        list(APPEND GIT_FILES "${GIT_COMMON_DIR}/${GIT_REF_PATH}")
    endif()

    set(GIT_DEPENDS)
    foreach(GIT_FILE IN LISTS GIT_FILES)
        if(EXISTS "${GIT_FILE}")
            list(APPEND GIT_DEPENDS "${GIT_FILE}")
        endif()
    endforeach()
    if(GIT_DEPENDS)
        # one property write: repeated set_source_files_properties calls
        # overwrite OBJECT_DEPENDS, keeping only the last file
        set_source_files_properties("${SOURCE_FILE}"
            PROPERTIES OBJECT_DEPENDS "${GIT_DEPENDS}")
    endif()
endfunction()
