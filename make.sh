#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="build"
BUILD_TYPE="${BUILD_TYPE:-Release}"

if [ "${EUID}" -eq 0 ]; then
    PREFIX="/usr"
else
    PREFIX="${PREFIX:-$HOME/.local}"
fi

echo "==> Configuring biwaytoolkit ($BUILD_TYPE) to prefix '$PREFIX'..."
cmake -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE="$BUILD_TYPE" -DCMAKE_INSTALL_PREFIX="$PREFIX" "$@"

echo "==> Building biwaytoolkit..."
cmake --build "$BUILD_DIR" -j"$(nproc)"

echo "==> Installing biwaytoolkit..."
cmake --install "$BUILD_DIR"

if [ "${EUID}" -eq 0 ]; then
    ldconfig || true
fi

echo "==> biwaytoolkit successfully built and installed to $PREFIX!"
