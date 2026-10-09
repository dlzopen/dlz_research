#!/bin/bash
# build_web.sh — Emscripten 构建脚本 / Emscripten build script
# 用法 / Usage: ./build_web.sh [debug|release]

set -e

BUILD_TYPE=${1:-debug}
BUILD_DIR="build_web"

echo "=========================================="
echo "  DLZ Web Build (Emscripten)"
echo "  Build type: $BUILD_TYPE"
echo "=========================================="

# 检查 Emscripten 环境 / Check Emscripten environment
if ! command -v emcmake &> /dev/null; then
    echo "Error: Emscripten not found in PATH"
    echo "Please install Emscripten and activate the environment:"
    echo "  source /path/to/emsdk/emsdk_env.sh"
    exit 1
fi

# 配置 / Configure
echo ""
echo "[1/2] Configuring..."
emcmake cmake -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=$BUILD_TYPE

# 构建 / Build
echo ""
echo "[2/2] Building..."
cmake --build "$BUILD_DIR" --config $BUILD_TYPE

echo ""
echo "=========================================="
echo "  Build complete!"
echo "  Output: $BUILD_DIR/bin/dlzr.html"
echo ""
echo "  To test locally:"
echo "    cd $BUILD_DIR/bin"
echo "    python -m http.server 8000"
echo "    # Open http://localhost:8000/dlzr.html"
echo "=========================================="
