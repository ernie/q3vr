#!/bin/sh

# Check if the nearest git tag has changed since last configure.
# version.txt contains e.g. "1.0.11_g22b63b92"; we compare just the semver part.
needs_reconfigure() {
  local build_dir="$1"
  local version_file="$build_dir/version.txt"
  [ -f "$version_file" ] || return 0
  local current_tag
  current_tag="$(git describe --tags --abbrev=0 2>/dev/null | sed 's/^v//')"
  [ -z "$current_tag" ] && return 1
  local cached_version
  cached_version="$(cut -d_ -f1 < "$version_file" 2>/dev/null)"
  [ "$current_tag" != "$cached_version" ]
}

# Configure on first run (fresh clone), reconfigure on version tag change.
if [ ! -f build/CMakeCache.txt ]; then
  echo "No configured build directory, configuring..."
  cmake -S . -B build || exit 1
elif needs_reconfigure build; then
  echo "Version tag changed, reconfiguring..."
  cmake build || exit 1
fi

cmake --build build --config Release || exit 1

# Build the MinGW configuration too if one has been set up
if [ -d build-mingw ]; then
  if needs_reconfigure build-mingw; then
    echo "Version tag changed, reconfiguring..."
    cmake build-mingw || exit 1
  fi
  cmake --build build-mingw || exit 1
fi

# Clean stray files that could overwrite real game assets
# (e.g., a demo pak0.pk3 replacing the actual one)
for dir in build/Release build-mingw/Release; do
  [ -d "$dir" ] || continue
  find "$dir" \( \
    -name "*.exp" \
    -o -name "*.lib" \
    -o -name "*.dll.a" \
    -o -name "*.txt" \
    -o -name "*.pk3" \) \
    -type f \
    -delete
done
