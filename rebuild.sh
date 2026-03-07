#!/bin/bash
set -e

# MuClaw Rebuild Script
echo "--- Rebuilding MuClaw ---"

# Ensure we use Clang as expected by Conan
export CC=/usr/bin/clang
export CXX=/usr/bin/clang++

# 1. Install dependencies via Conan
echo "Installing dependencies via Conan..."
# Clear build dir to avoid cached compiler issues
rm -rf build
conan install . --output-folder=build --build=missing

# 2. Configure CMake using the generated preset, forcing Clang
echo "Configuring CMake..."
cmake --preset conan-release -DCMAKE_C_COMPILER=/usr/bin/clang -DCMAKE_CXX_COMPILER=/usr/bin/clang++

# 3. Build
echo "Building..."
cmake --build --preset conan-release

echo "--- Build Complete ---"
echo "You can run the program with: ./build/Release/muclaw"
