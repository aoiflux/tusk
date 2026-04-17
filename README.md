# tusk

A disk-image analysis library and tool built on top of [The Sleuth Kit (libtsk)](https://www.sleuthkit.org/) and zlib.

Two platform builds are provided:

| Directory | Platform | Build system |
|-----------|----------|-------------|
| `lnx/`    | Linux    | Make (via CMake) |
| `win/`    | Windows  | CMake → Visual Studio |

---

## Linux (`lnx/`)

### Prerequisites

Install the Sleuth Kit and zlib development packages via apt:

```bash
sudo apt update
sudo apt install libtsk-dev zlib1g-dev build-essential cmake
```

The build uses **statically compiled** copies of `libtsk` and `zlib` placed in `lnx/lib/`:

```
lnx/lib/
    libtsk.a
    libz.a
```

These static archives are linked into the output binaries so that the resulting ELF files have no runtime dependency on the system copies of those libraries. Obtain the static archives by either:

- Compiling libtsk and zlib from source with static library output, **or**
- Extracting the `.a` files that the dev packages install (typically under `/usr/lib/x86_64-linux-gnu/` or similar).

The Sleuth Kit headers are expected at the path set in `CMakeLists.txt`:

```cmake
set(FOR_INC_DIR "/home/mew/tusk/include")
```

Adjust this variable (and `FOR_LIB_DIR`) in `lnx/CMakeLists.txt` to match your local paths before building.

### Build

```bash
cd lnx
mkdir build && cd build
cmake ..
make
```

CMake build options (all `ON` by default):

| Option | Description |
|--------|-------------|
| `BUILD_WRAPPER` | Build the libtusk wrapper library |
| `BUILD_STATIC_WRAPPER` | Build `libtusk.a` static archive |
| `BUILD_SHARED_WRAPPER` | Build `libtusk.so` shared library |
| `BUILD_TOOL` | Build the `tsktool` standalone ELF binary |

To build only the static library and tool, for example:

```bash
cmake -DBUILD_SHARED_WRAPPER=OFF ..
make
```

### Outputs

| Output | Description |
|--------|-------------|
| `build/libtusk.a` | Static library (linked against static libtsk + zlib) |
| `build/libtusk.so` / `libtusk.so.1` | Shared library (linked against system shared libtsk + zlib) |
| `build/tsktool` | Fully static ELF binary (`-static -static-libgcc -static-libstdc++`) |

### Test

Compile the test program against the built static library:

```bash
gcc test_libtusk.c -o test_libtusk -I include -L build -ltusk -lpthread
./test_libtusk /path/to/disk.img
```

---

## Windows (`win/`)

### Prerequisites

1. **Visual Studio 2019 or later** with the *Desktop development with C++* workload installed.
2. **CMake 3.16+** (bundled with Visual Studio or from https://cmake.org/download/).
3. **The Sleuth Kit headers** — clone or download the Sleuth Kit source and note the path to its `include/` directory (referred to below as `TSK_INC`).

The build uses **pre-built static `.lib` files** for libtsk and zlib placed in `win/lib/`:

```
win/lib/
    libtsk.lib
    zlib.lib
```

Obtain these by building The Sleuth Kit and zlib from source targeting a static (`/MT`) Release configuration, or from a trusted pre-built distribution.

The Sleuth Kit include path is hard-coded in `win/CMakeLists.txt`:

```cmake
set(TSK_INC "O:\\tmp\\sleuthkit")
```

Update this line to point to your local Sleuth Kit headers before generating the solution.

### Generate the Visual Studio Solution

Open a **Developer Command Prompt** (or any shell with CMake on PATH) and run:

```bat
cd win
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64
```

Replace `"Visual Studio 17 2022"` with your installed version if different (e.g. `"Visual Studio 16 2019"`).

This generates `win/build/tusk.sln`.

### Build in Visual Studio

1. Open `win/build/tusk.sln` in Visual Studio.
2. Set the solution configuration to **Debug** or **Release** as desired.
3. Build → Build Solution (`Ctrl+Shift+B`).

The MSVC runtime is set to **MultiThreaded** (`/MT`) so the outputs have no CRT DLL dependency.

### Outputs

| Output | Description |
|--------|-------------|
| `build/Debug/libtusk.lib` | Static library |
| `build/Debug/tsktool.exe` | Standalone executable |

### Test

Compile the test program from a Developer Command Prompt:

```bat
cd win
cl test_libtusk.c /I include /I "O:\tmp\sleuthkit" ^
    /link build\Debug\libtusk.lib lib\libtsk.lib lib\zlib.lib
test_libtusk.exe C:\path\to\disk.img
```

---

## Project Layout

```
tusk/
├── lnx/
│   ├── CMakeLists.txt        # Linux build configuration
│   ├── libtusk.cpp           # Library implementation
│   ├── tusk.cpp              # tsktool entry point
│   ├── test_libtusk.c        # Test harness
│   ├── LIBTUSK_API.md        # Public API documentation
│   ├── include/
│   │   └── libtusk.h         # Public header
│   └── lib/
│       ├── libtsk.a          # Static Sleuth Kit (pre-built)
│       └── libz.a            # Static zlib (pre-built)
└── win/
    ├── CMakeLists.txt        # Windows / MSVC build configuration
    ├── libtusk.cpp           # Library implementation
    ├── tusk.cpp              # tsktool entry point
    ├── test_libtusk.c        # Test harness
    ├── LIBTUSK_API.md        # Public API documentation
    ├── include/
    │   ├── libtusk.h         # Public header
    │   └── msvc_compat/
    │       └── inttypes.h    # MSVC compatibility shim
    ├── lib/
    │   ├── libtsk.lib        # Static Sleuth Kit (pre-built)
    │   └── zlib.lib          # Static zlib (pre-built)
    └── build/                # CMake-generated VS solution (generated)
```

## Public API

See `LIBTUSK_API.md` in either platform directory for the full specification. The two exported functions are:

```c
// Analyze a disk image; returns a NUL-terminated JSON string on success, NULL on failure.
char *libtusk_analyze(const char *image_path);

// Release the string returned by libtusk_analyze. Do NOT use free() directly.
void libtusk_free(char *ptr);
```
