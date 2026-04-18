<div align="center">
  <img src="logo.svg" alt="tusk logo" width="220"/>
</div>

# tusk

A **C ABI wrapper library and tool** around
[The Sleuth Kit (libtsk)](https://www.sleuthkit.org/) and zlib.

`tusk` bridges native disk-forensic power to any language that can speak **FFI,
CGO, or C bindings** — Go, Rust, Python, C#, Java, and beyond — without
requiring callers to deal with libtsk's raw C++ ABI directly. It exposes a
clean, stable C ABI surface (`libtusk`) that higher-level runtimes can load and
call without friction, while the bundled `tsktool` binary covers direct
command-line forensic workloads.

Two platform builds are provided:

| Directory | Platform | Build system               |
| --------- | -------- | -------------------------- |
| `lnx/`    | Linux    | CMake + Ninja (or make)    |
| `win/`    | Windows  | CMake → Visual Studio 2022 |

---

## Linux (`lnx/`)

### Prerequisites

```bash
sudo apt update
sudo apt install cmake build-essential libtsk-dev zlib1g-dev
# Recommended for faster builds:
sudo apt install ninja-build
```

Place static library archives in `lnx/lib/` before building:

```
lnx/lib/
    libtsk.a
    libz.a
```

Obtain the static archives by compiling libtsk/zlib from source or extracting
them from the dev packages (typically under `/usr/lib/x86_64-linux-gnu/`).

Sleuth Kit headers are expected in `lnx/include/` (or wherever `FOR_INC_DIR`
points in `lnx/CMakeLists.txt`). Adjust the variable if your layout differs.

### Build

Run the generate script — it handles everything (CMake configure + compile):

```bash
chmod +x lnx/generate.sh
./lnx/generate.sh
```

The script auto-detects Ninja and uses it when available, falling back to
`make`. Both run in parallel across all available CPU cores. If neither is found
it exits with a clear error.

To build manually without the script:

```bash
cd lnx
mkdir -p build && cd build
cmake ..
make -j$(nproc)
```

#### CMake build options

All options are `ON` by default and can be toggled at configure time:

| Option                 | Description                               |
| ---------------------- | ----------------------------------------- |
| `BUILD_WRAPPER`        | Build the libtusk wrapper library         |
| `BUILD_STATIC_WRAPPER` | Build `libtusk.a` static archive          |
| `BUILD_SHARED_WRAPPER` | Build `libtusk.so` shared library         |
| `BUILD_TOOL`           | Build the `tsktool` standalone ELF binary |

Example — static library and tool only:

```bash
cmake -DBUILD_SHARED_WRAPPER=OFF ..
```

### Outputs

| Output                                  | Description                                                   |
| --------------------------------------- | ------------------------------------------------------------- |
| `lnx/build/libtusk.a`                   | Static library                                                |
| `lnx/build/libtusk.so` / `libtusk.so.1` | Shared library                                                |
| `lnx/build/tsktool`                     | Fully static ELF (`-static -static-libgcc -static-libstdc++`) |

### Test

```bash
gcc lnx/test_libtusk.c -o test_libtusk -I lnx/include -L lnx/build -ltusk -lpthread
./test_libtusk /path/to/disk.img
```

---

## Windows (`win/`)

### Prerequisites

1. **Visual Studio 2022** with the _Desktop development with C++_ workload.
2. **CMake 3.16+** — bundled with Visual Studio or from
   [cmake.org](https://cmake.org/download/).
3. **Sleuth Kit headers** — set `TSK_INC` in `win/CMakeLists.txt` to point at
   your local copy.

Place pre-built static libraries in `win/lib/` before building:

```
win/lib/
    libtsk.lib
    zlib.lib
```

### Generate the Visual Studio solution

```powershell
cd win
.\generate.ps1
```

This creates `win/build/` if needed and runs `cmake --preset windows-vs2022`,
generating `win/build/tusk.sln` targeting x64.

To generate manually:

```powershell
cd win
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64
```

### Build in Visual Studio

1. Open `win/build/tusk.sln`.
2. Choose **Debug** or **Release**.
3. **Build → Build Solution** (`Ctrl+Shift+B`).

The MSVC runtime is set to **MultiThreaded** (`/MT`) — no CRT DLL dependency.

### Outputs

| Output                        | Description           |
| ----------------------------- | --------------------- |
| `win/build/Debug/libtusk.lib` | Static library        |
| `win/build/Debug/tsktool.exe` | Standalone executable |

### Test

```bat
cd win
cl test_libtusk.c /I include /I "%TSK_INC%" ^
    /link build\Debug\libtusk.lib lib\libtsk.lib lib\zlib.lib
test_libtusk.exe C:\path\to\disk.img
```

---

## Project layout

```
tusk/
├── lnx/
│   ├── CMakeLists.txt        # Linux build configuration
│   ├── CMakePresets.json     # Ninja + Unix Makefiles presets
│   ├── generate.sh           # One-shot configure + build script
│   ├── libtusk.cpp           # Library implementation
│   ├── tusk.cpp              # tsktool entry point
│   ├── test_libtusk.c        # Test harness
│   ├── LIBTUSK_ABI.md        # Public ABI documentation
│   ├── include/
│   │   └── libtusk.h         # Public header
│   └── lib/
│       ├── libtsk.a          # Static Sleuth Kit (pre-built)
│       └── libz.a            # Static zlib (pre-built)
└── win/
    ├── CMakeLists.txt        # Windows / MSVC build configuration
    ├── CMakePresets.json     # Visual Studio 2022 x64 preset
    ├── generate.ps1          # One-shot solution generation script
    ├── libtusk.cpp           # Library implementation
    ├── tusk.cpp              # tsktool entry point
    ├── test_libtusk.c        # Test harness
    ├── LIBTUSK_ABI.md        # Public ABI documentation
    ├── include/
    │   ├── libtusk.h         # Public header
    │   └── msvc_compat/
    │       └── inttypes.h    # MSVC compatibility shim
    └── lib/
        ├── libtsk.lib        # Static Sleuth Kit (pre-built)
        └── zlib.lib          # Static zlib (pre-built)
```

---

## Public ABI

See `LIBTUSK_ABI.md` in either platform directory for the full specification.

```c
// Analyze a disk image; returns a NUL-terminated JSON string on success, NULL on failure.
char *libtusk_analyze(const char *image_path);

// Release the string returned by libtusk_analyze. Do NOT use free() directly.
void libtusk_free(char *ptr);
```
