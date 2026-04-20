#!/bin/sh
# Cross-compile libtsk as a static GNU COFF (.a) library using MinGW.
# Usage: sh build-mingw-static.sh [32|64]   (default: 64)

set -e

# ---------------------------------------------------------------------------
# 1. Determine target architecture and MinGW triplet
# ---------------------------------------------------------------------------
ARCH="${1:-64}"

case "$ARCH" in
    32) MINGW_TRIPLET="i686-w64-mingw32"   ;;
    64) MINGW_TRIPLET="x86_64-w64-mingw32" ;;
    *)
        printf 'Usage: %s [32|64]\n' "$0" >&2
        exit 1
        ;;
esac

# Allow the caller to override the triplet entirely.
: "${CROSS_TRIPLET:=$MINGW_TRIPLET}"

# ---------------------------------------------------------------------------
# 2. Verify that the cross-toolchain is present
# ---------------------------------------------------------------------------
CC_CROSS="${CROSS_TRIPLET}-gcc"
CXX_CROSS="${CROSS_TRIPLET}-g++"
AR_CROSS="${CROSS_TRIPLET}-ar"
RANLIB_CROSS="${CROSS_TRIPLET}-ranlib"
STRIP_CROSS="${CROSS_TRIPLET}-strip"

for tool in "$CC_CROSS" "$CXX_CROSS" "$AR_CROSS" "$RANLIB_CROSS"; do
    if ! command -v "$tool" >/dev/null 2>&1; then
        printf 'ERROR: %s not found in PATH.\n' "$tool" >&2
        printf 'Install the MinGW-w64 cross-compiler, e.g.:\n' >&2
        printf '  apt-get install gcc-mingw-w64 g++-mingw-w64\n' >&2
        exit 1
    fi
done

# ---------------------------------------------------------------------------
# 3. Paths
# ---------------------------------------------------------------------------
SRCDIR="$(cd "$(dirname "$0")" && pwd)"
BUILDDIR="${SRCDIR}/_build_mingw${ARCH}"
PREFIX="${SRCDIR}/_install_mingw${ARCH}"

printf '==> Source  : %s\n' "$SRCDIR"
printf '==> Build   : %s\n' "$BUILDDIR"
printf '==> Prefix  : %s\n' "$PREFIX"
printf '==> Triplet : %s\n' "$CROSS_TRIPLET"

# ---------------------------------------------------------------------------
# 4. Bootstrap autotools if configure does not yet exist
#
#   We call autoreconf directly rather than ./bootstrap because the project's
#   bootstrap script invokes `libtoolize --force` which errors when both
#   AC_CONFIG_MACRO_DIRS([m4]) and ACLOCAL_AMFLAGS=-I m4 are present.
#   autoreconf handles that overlap gracefully.
# ---------------------------------------------------------------------------
if [ ! -f "${SRCDIR}/configure" ]; then
    printf '==> Running autoreconf (bootstrap)\n'
    if ! command -v autoreconf >/dev/null 2>&1; then
        printf 'ERROR: autoreconf not found. Install autoconf/automake/libtool.\n' >&2
        exit 1
    fi
    cd "$SRCDIR"
    autoreconf --install --force
fi

# ---------------------------------------------------------------------------
# 5. Create an out-of-tree build directory
# ---------------------------------------------------------------------------
mkdir -p "$BUILDDIR"
cd "$BUILDDIR"

# ---------------------------------------------------------------------------
# 6. Configure
#
#   --host                   cross-compilation target
#   --enable-static          produce a .a archive
#   --disable-shared         do not build a .dll/.so
#   --disable-java           skip JNI/JAR (requires a JDK in the host sysroot)
#   --without-afflib         optional; skip unless pre-built for MinGW
#   --without-libewf         optional
#   --without-libvhdi        optional
#   --without-libvmdk        optional
#   --without-libvslvm       optional
#   --without-libbfio        optional (libvslvm depends on it)
#   --disable-cppunit        no unit-test framework needed for a library build
#   --disable-multithreading avoids a pthreads dependency; Windows uses its
#                            own threading; re-enable if you supply winpthreads
# ---------------------------------------------------------------------------
printf '==> Running configure\n'

"${SRCDIR}/configure" \
    --host="${CROSS_TRIPLET}" \
    --build="$(${SRCDIR}/config/config.guess 2>/dev/null || gcc -dumpmachine)" \
    --prefix="${PREFIX}" \
    --enable-static \
    --disable-shared \
    --disable-java \
    --without-afflib \
    --without-libewf \
    --without-libvhdi \
    --without-libvmdk \
    --without-libvslvm \
    --without-libbfio \
    --disable-cppunit \
    --disable-multithreading \
    CC="$CC_CROSS" \
    CXX="$CXX_CROSS" \
    AR="$AR_CROSS" \
    RANLIB="$RANLIB_CROSS" \
    STRIP="$STRIP_CROSS" \
    CFLAGS="-O2 -Wno-format" \
    CXXFLAGS="-O2 -Wno-format" \
    LDFLAGS="-static-libgcc -static-libstdc++"

# ---------------------------------------------------------------------------
# 7. Build only the tsk/ sub-tree (the library itself, not the CLI tools)
# ---------------------------------------------------------------------------
printf '==> Building libtsk\n'
NCPU=$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 2)
make -j"$NCPU" -C tsk

# ---------------------------------------------------------------------------
# 8. Extract the final thin .a archives into one fat archive
#    libtool puts the real objects inside tsk/.libs/
# ---------------------------------------------------------------------------
printf '==> Merging sub-archives into libtsk.a\n'

OUTDIR="${SRCDIR}/_out_mingw${ARCH}"
mkdir -p "$OUTDIR"
OUTLIB="${OUTDIR}/libtsk.a"

# Collect every .a that libtool produced for the sub-libraries
SUBLIBS=$(find "${BUILDDIR}/tsk" -name '*.a' ! -name 'libtsk.a' 2>/dev/null | sort)

# libtool also produces a convenience archive directly
LTLIB="${BUILDDIR}/tsk/.libs/libtsk.a"
if [ -f "$LTLIB" ]; then
    cp "$LTLIB" "$OUTLIB"
    printf '==> Copied libtool archive: %s\n' "$LTLIB"
else
    # Fallback: merge sub-archives using ar -M (MRI scripting)
    TMPDIR_OBJS="${BUILDDIR}/_merge_tmp"
    rm -rf "$TMPDIR_OBJS"
    mkdir -p "$TMPDIR_OBJS"

    MRI_SCRIPT="${TMPDIR_OBJS}/merge.mri"
    printf 'CREATE %s\n' "$OUTLIB" > "$MRI_SCRIPT"
    for lib in $SUBLIBS; do
        printf 'ADDLIB %s\n' "$lib" >> "$MRI_SCRIPT"
    done
    printf 'SAVE\nEND\n' >> "$MRI_SCRIPT"

    "$AR_CROSS" -M < "$MRI_SCRIPT"
fi

"$RANLIB_CROSS" "$OUTLIB"

# ---------------------------------------------------------------------------
# 9. Copy public headers
# ---------------------------------------------------------------------------
printf '==> Installing headers to %s\n' "$PREFIX"
make -C tsk install-pkgincludeHEADERS \
    pkgincludedir="${PREFIX}/include/tsk" \
    || true   # non-fatal; headers can be used from source tree as well

# ---------------------------------------------------------------------------
# 10. Summary
# ---------------------------------------------------------------------------
LIBSIZE=$(wc -c < "$OUTLIB" 2>/dev/null || echo '?')
printf '\n'
printf '===================================================\n'
printf ' Build complete\n'
printf '  Archive : %s\n' "$OUTLIB"
printf '  Size    : %s bytes\n' "$LIBSIZE"
printf '  Headers : %s/include/tsk  (or use %s/tsk/)\n' "$PREFIX" "$SRCDIR"
printf '===================================================\n'
