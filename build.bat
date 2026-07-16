@echo off
setlocal enabledelayedexpansion

:: Configure on first run (fresh clone), reconfigure on version tag change.
:: version.txt contains e.g. "1.0.11_g22b63b92"; we compare just the semver part.
if not exist build\CMakeCache.txt (
    echo No configured build directory, configuring...
    cmake -S . -B build || exit /b 1
) else (
    set "NEEDS_RECONFIG=0"
    if not exist build\version.txt (
        set "NEEDS_RECONFIG=1"
    ) else (
        for /f "tokens=1 delims=_" %%v in (build\version.txt) do set "CACHED_VERSION=%%v"
        for /f "delims=" %%t in ('git describe --tags --abbrev^=0 2^>nul') do set "RAW_TAG=%%t"
        if defined RAW_TAG (
            set "CURRENT_TAG=!RAW_TAG:~1!"
            if not "!CURRENT_TAG!"=="!CACHED_VERSION!" set "NEEDS_RECONFIG=1"
        )
    )
    if "!NEEDS_RECONFIG!"=="1" (
        echo Version tag changed, reconfiguring...
        cmake build || exit /b 1
    )
)

cmake --build build --config Release || exit /b 1

:: Clean stray files that could overwrite real game assets
:: (e.g., a demo pak0.pk3 replacing the actual one)
for /r build\Release %%f in (*.exp *.lib *.txt *.pk3) do (
    del "%%f"
)
