#!/usr/bin/env bash
set -euo pipefail

# ==============================================================================
# build_macos.sh - macOS 自动构建、签名与 DMG 打包脚本
# 适用产品: SHP轻量查看器 (ShpLightViewer)
# ==============================================================================

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
BUILD_DIR="${PROJECT_ROOT}/build"
PACKAGE_DIR="${BUILD_DIR}/package_macos"
OUTPUTS_DIR="${PROJECT_ROOT}/outputs"

APP_NAME="ShpLightViewer"
DMG_NAME="ShpLightViewer-0.1.0-macOS-arm64.dmg"

echo "=== 1. 清理并准备构建目录 ==="
mkdir -p "${OUTPUTS_DIR}"
rm -rf "${PACKAGE_DIR}"
mkdir -p "${PACKAGE_DIR}"

echo "=== 2. 检查 Qt6 环境 ==="
QT_DIR="${QT_DIR:-$(brew --prefix qt 2>/dev/null || echo "/opt/homebrew/opt/qt")}"
if [ ! -d "${QT_DIR}" ]; then
    echo "错误: 未找到 Qt6 安装目录 (${QT_DIR})" >&2
    exit 1
fi
echo "Qt6 目录: ${QT_DIR}"

MACDEPLOYQT_BIN="${QT_DIR}/bin/macdeployqt"
if [ ! -f "${MACDEPLOYQT_BIN}" ]; then
    echo "错误: 未找到 macdeployqt 工具 (${MACDEPLOYQT_BIN})" >&2
    exit 1
fi

echo "=== 3. CMake 配置与编译 ==="
cmake -B "${BUILD_DIR}" -S "${PROJECT_ROOT}" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_PREFIX_PATH="${QT_DIR}"

cmake --build "${BUILD_DIR}" --config Release -j"$(sysctl -n hw.ncpu)"

echo "=== 4. 拷贝 App Bundle 到打包目录 ==="
BUILT_APP="${BUILD_DIR}/${APP_NAME}.app"
if [ ! -d "${BUILT_APP}" ]; then
    echo "错误: 未找到编译生成的 App: ${BUILT_APP}" >&2
    exit 1
fi

cp -R "${BUILT_APP}" "${PACKAGE_DIR}/"
TARGET_APP="${PACKAGE_DIR}/${APP_NAME}.app"

echo "=== 5. 使用 macdeployqt 嵌入 Qt 框架与插件 ==="
"${MACDEPLOYQT_BIN}" "${TARGET_APP}" -always-overwrite -verbose=1

echo "=== 6. 执行 Ad-hoc 代码签名 ==="
# 递归签名所有动态库与插件，最后签名主程序 Bundle
find "${TARGET_APP}/Contents/Frameworks" -type f \( -name "*.dylib" -o -name "*.so" \) -exec codesign --force --verify --verbose --sign - {} \; 2>/dev/null || true
find "${TARGET_APP}/Contents/PlugIns" -type f \( -name "*.dylib" -o -name "*.so" \) -exec codesign --force --verify --verbose --sign - {} \; 2>/dev/null || true
codesign --force --deep --verify --verbose --sign - "${TARGET_APP}"

codesign --verify --deep --strict --verbose=2 "${TARGET_APP}"
echo "Ad-hoc 签名校验通过！"

echo "=== 7. 生成并归档 DMG 产物 ==="
DMG_PATH="${OUTPUTS_DIR}/${DMG_NAME}"
rm -f "${DMG_PATH}"

hdiutil create -volname "SHP轻量查看器" \
    -srcfolder "${PACKAGE_DIR}" \
    -ov -format UDZO \
    "${DMG_PATH}"

echo "=== 8. 验证 DMG 完整性 ==="
hdiutil verify "${DMG_PATH}"

echo "macOS DMG 打包成功！产物位于: ${DMG_PATH}"
