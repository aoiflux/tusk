set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# Zig wrapper scripts sit alongside this toolchain file.
# They forward all arguments to the appropriate zig sub-command so that CMake
# sees a standard compiler/archiver interface without needing to know about
# zig's multi-tool entry point.
set(_ZIG_SCRIPTS_DIR "${CMAKE_CURRENT_LIST_DIR}")

set(CMAKE_C_COMPILER   "${_ZIG_SCRIPTS_DIR}/zig-cc.sh")
set(CMAKE_CXX_COMPILER "${_ZIG_SCRIPTS_DIR}/zig-c++.sh")
set(CMAKE_AR           "${_ZIG_SCRIPTS_DIR}/zig-ar.sh")
set(CMAKE_RANLIB       "${_ZIG_SCRIPTS_DIR}/zig-ranlib.sh")

# zig cc presents itself as Clang; keep find_* from searching into the host
# sysroot so that all libs/includes resolve from the project tree only.
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
