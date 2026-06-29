#!/bin/sh
# Regenerates the Xcode project from CMakeLists.txt.
# Run from anywhere inside the repo.

set -eu

REPO_ROOT="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$REPO_ROOT/framework/build_xcode"

echo "Regenerating Xcode project..."
mkdir -p "$BUILD_DIR"
rm -f "$BUILD_DIR/CMakeCache.txt"
rm -rf "$BUILD_DIR/CMakeFiles" "$BUILD_DIR/framework.xcodeproj"
cmake -S "$REPO_ROOT" -B "$BUILD_DIR" -G Xcode -Wno-dev

if [ $? -eq 0 ]; then
    echo "Done. Project at: $BUILD_DIR/framework.xcodeproj"
    echo "Reload changes in Xcode if the project is already open."
else
    echo "cmake failed." >&2
    exit 1
fi
