#!/bin/sh
set -eu

CYAN='\033[0;36m'
DCYAN='\033[0;34m'
YELLOW='\033[0;33m'
ORANGE='\033[0;33m'
GREEN='\033[0;32m'
RED='\033[0;31m'
GRAY='\033[0;90m'
RESET='\033[0m'

LINE='──────────────────────────────────────────────────────'

# ─── Argument parsing ────────────────────────────────────────────────────────

CROSS_MINGW=0
ZIG=0
for arg in "$@"; do
    case "$arg" in
        --mingw) CROSS_MINGW=1 ;;
        --zig)   ZIG=1 ;;
        *) printf "\n  ${RED}✘  Unknown argument: %s${RESET}\n\n" "$arg"; exit 1 ;;
    esac
done

if [ "$CROSS_MINGW" -eq 1 ] && [ "$ZIG" -eq 1 ]; then
    printf "\n  ${RED}✘  --mingw and --zig are mutually exclusive.${RESET}\n\n"
    exit 1
fi

# ─── Dependency checks ───────────────────────────────────────────────────────

if ! command -v cmake >/dev/null 2>&1; then
    printf "\n  ${RED}✘  cmake not found. Please install cmake and try again.${RESET}\n\n"
    exit 1
fi

if [ "$CROSS_MINGW" -eq 1 ]; then
    if ! command -v x86_64-w64-mingw32-gcc >/dev/null 2>&1; then
        printf "\n  ${RED}✘  x86_64-w64-mingw32-gcc not found. Install mingw-w64 and try again.${RESET}\n\n"
        exit 1
    fi
    PRESET='mingw-cross'
    BUILD_TOOL_LABEL='MinGW cross-compile'
    if command -v ninja >/dev/null 2>&1; then
        JOBS="$(nproc)"
        BUILD_CMD="ninja -j$JOBS"
    else
        printf "  ${ORANGE}⚠  ninja not found — falling back to make.${RESET}\n\n"
        if ! command -v make >/dev/null 2>&1; then
            printf "  ${RED}✘  make not found. Please install make or ninja-build.${RESET}\n\n"
            exit 1
        fi
        JOBS="$(nproc)"
        BUILD_CMD="make -j$JOBS"
    fi
elif [ "$ZIG" -eq 1 ]; then
    if ! command -v zig >/dev/null 2>&1; then
        printf "\n  ${RED}✘  zig not found in PATH. Please install Zig and try again.${RESET}\n\n"
        exit 1
    fi
    if ! command -v ninja >/dev/null 2>&1; then
        printf "\n  ${RED}✘  ninja not found. The zig-linux preset requires Ninja. Install ninja-build and try again.${RESET}\n\n"
        exit 1
    fi
    # Ensure wrapper scripts are executable (they may lose the bit after a git clone on some systems)
    SCRIPT_DIR_EARLY="$(cd "$(dirname "$0")" && pwd)"
    chmod +x \
        "$SCRIPT_DIR_EARLY/cmake/zig-cc.sh" \
        "$SCRIPT_DIR_EARLY/cmake/zig-c++.sh" \
        "$SCRIPT_DIR_EARLY/cmake/zig-ar.sh" \
        "$SCRIPT_DIR_EARLY/cmake/zig-ranlib.sh"
    PRESET='zig-linux'
    BUILD_TOOL_LABEL='Zig'
    JOBS="$(nproc)"
    BUILD_CMD="ninja -j$JOBS"
elif command -v ninja >/dev/null 2>&1; then
    PRESET='linux-ninja'
    JOBS="$(nproc)"
    BUILD_CMD="ninja -j$JOBS"
    BUILD_TOOL_LABEL='Ninja'
else
    printf "  ${ORANGE}⚠  ninja not found — falling back to make (slower builds).${RESET}\n"
    printf "  ${GRAY}   Install ninja-build for faster incremental compilation.${RESET}\n\n"
    if ! command -v make >/dev/null 2>&1; then
        printf "  ${RED}✘  make not found either. Please install make or ninja-build.${RESET}\n\n"
        exit 1
    fi
    JOBS="$(nproc)"
    PRESET='linux-makefiles'
    BUILD_CMD="make -j$JOBS"
    BUILD_TOOL_LABEL='make'
fi

# ─── Banner ──────────────────────────────────────────────────────────────────

printf "\n  ${DCYAN}%s${RESET}\n" "$LINE"
printf "   ${CYAN}TUSK  •  CMake Configure  •  %s${RESET}\n" "$BUILD_TOOL_LABEL"
printf "  ${DCYAN}%s${RESET}\n\n" "$LINE"

# ─── Configure ───────────────────────────────────────────────────────────────

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
if [ "$CROSS_MINGW" -eq 1 ]; then
    BUILD_DIR="$SCRIPT_DIR/build-mingw"
elif [ "$ZIG" -eq 1 ]; then
    BUILD_DIR="$SCRIPT_DIR/build-zig"
else
    BUILD_DIR="$SCRIPT_DIR/build"
fi

printf "  ${YELLOW}►  Creating build directory...${RESET}\n"
mkdir -p "$BUILD_DIR"

printf "  ${YELLOW}►  Running: cmake --preset %s${RESET}\n\n" "$PRESET"
cd "$SCRIPT_DIR"

if ! cmake --preset "$PRESET"; then
    printf "\n  ${RED}✘  cmake configuration failed.${RESET}\n\n"
    exit 1
fi

printf "\n  ${GREEN}✔  Configuration complete.${RESET}\n"
printf "  ${YELLOW}►  Building with %s using %s jobs...${RESET}\n\n" "$BUILD_TOOL_LABEL" "$JOBS"
cd "$BUILD_DIR"

if $BUILD_CMD; then
    printf "\n  ${GREEN}✔  Build complete.${RESET}\n\n"
else
    printf "\n  ${RED}✘  Build failed.${RESET}\n\n"
    exit 1
fi
