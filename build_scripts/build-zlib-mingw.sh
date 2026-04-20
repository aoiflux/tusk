#!/usr/bin/env bash
set -euo pipefail

# Cross-compile zlib as a static library for Windows using MinGW-w64
# Target: x86_64-w64-mingw32
# Output:  build-mingw/libz.a  +  headers

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build-mingw"
INSTALL_DIR="${BUILD_DIR}/install"

TOOLCHAIN_PREFIX="x86_64-w64-mingw32"
CC="${TOOLCHAIN_PREFIX}-gcc"
AR="${TOOLCHAIN_PREFIX}-ar"
RANLIB="${TOOLCHAIN_PREFIX}-ranlib"
STRIP="${TOOLCHAIN_PREFIX}-strip"

# ---------------------------------------------------------------------------
# Sanity checks
# ---------------------------------------------------------------------------
for tool in "$CC" "$AR" "$RANLIB" cmake; do
    if ! command -v "$tool" &>/dev/null; then
        echo "ERROR: '$tool' not found. Install mingw-w64 and cmake." >&2
        exit 1
    fi
done

echo "==> Compiler : $(command -v "$CC")"
echo "==> Build dir: ${BUILD_DIR}"
echo "==> Install  : ${INSTALL_DIR}"

# ---------------------------------------------------------------------------
# CMake toolchain file (written to a temp location inside build dir)
# ---------------------------------------------------------------------------
mkdir -p "${BUILD_DIR}"
TOOLCHAIN_FILE="${BUILD_DIR}/mingw-toolchain.cmake"

cat > "${TOOLCHAIN_FILE}" <<EOF
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

set(CMAKE_C_COMPILER   ${TOOLCHAIN_PREFIX}-gcc)
set(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}-g++)
set(CMAKE_RC_COMPILER  ${TOOLCHAIN_PREFIX}-windres)
set(CMAKE_AR           $(command -v ${AR}))
set(CMAKE_RANLIB       $(command -v ${RANLIB}))

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
EOF

# ---------------------------------------------------------------------------
# Configure
# ---------------------------------------------------------------------------
cmake -S "${SCRIPT_DIR}" -B "${BUILD_DIR}" \
    -DCMAKE_TOOLCHAIN_FILE="${TOOLCHAIN_FILE}" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="${INSTALL_DIR}" \
    -DZLIB_BUILD_STATIC=ON \
    -DZLIB_BUILD_SHARED=OFF \
    -DZLIB_BUILD_TESTING=OFF \
    -DZLIB_INSTALL=ON

# ---------------------------------------------------------------------------
# Build
# ---------------------------------------------------------------------------
cmake --build "${BUILD_DIR}" --config Release --parallel "$(nproc)"

# ---------------------------------------------------------------------------
# Install headers + library
# ---------------------------------------------------------------------------
cmake --install "${BUILD_DIR}" --config Release

echo ""
echo "==> Done."
echo "    Static library : ${INSTALL_DIR}/lib/libzs.a"
echo "    Headers        : ${INSTALL_DIR}/include/"
