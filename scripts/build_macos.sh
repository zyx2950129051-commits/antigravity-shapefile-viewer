#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

echo "=== 1. 构建 Release 版本 ==="
cmake -B "${ROOT_DIR}/build" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_PREFIX_PATH="/opt/homebrew/opt/qt"
cmake --build "${ROOT_DIR}/build" --config Release -j"$(sysctl -n hw.ncpu)"

echo "=== 2. 运行自动化测试 ==="
ctest --test-dir "${ROOT_DIR}/build" --output-on-failure

echo "=== 3. 准备独立 Application Bundle ==="
PKG_DIR="${ROOT_DIR}/build/package_macos"
rm -rf "${PKG_DIR}"
mkdir -p "${PKG_DIR}"
mkdir -p "${ROOT_DIR}/outputs"

APP_PATH="${PKG_DIR}/ShapefileViewer.app"
cp -R "${ROOT_DIR}/build/ShapefileViewer.app" "${APP_PATH}"

echo "=== 4. 运行 macdeployqt 收集 Qt 框架和插件 ==="
/opt/homebrew/opt/qt/bin/macdeployqt "${APP_PATH}" -verbose=1 -no-codesign

echo "=== 5. 校验 Bundle 动态依赖 ==="
MAIN_BIN="${APP_PATH}/Contents/MacOS/ShapefileViewer"
echo "检查主二进制依赖库："
otool -L "${MAIN_BIN}"

if otool -L "${MAIN_BIN}" | grep -q "/opt/homebrew/opt/qt"; then
    echo "错误：存在未被 @executable_path/@rpath 替换的绝对路径！"
    exit 1
else
    echo "依赖检查通过：所有 Qt 框架均已指向内嵌相对路径。"
fi

echo "=== 6. 执行 Ad-hoc 签名 ==="
# 对内部所有动态库与 Frameworks 进行签名
find "${APP_PATH}/Contents/Frameworks" -type f \( -name "*.dylib" -o -perm +111 \) -exec codesign --force --sign - {} + 2>/dev/null || true
find "${APP_PATH}/Contents/PlugIns" -type f \( -name "*.dylib" -o -perm +111 \) -exec codesign --force --sign - {} + 2>/dev/null || true
codesign --force --deep --sign - "${APP_PATH}"
codesign -vvv --deep --strict "${APP_PATH}"
echo "Ad-hoc 签名校验通过！"

echo "=== 7. 生成并归档 DMG 产物 ==="
FINAL_DMG="${ROOT_DIR}/outputs/ShapefileViewer-0.1.0-macOS-arm64.dmg"
rm -f "${FINAL_DMG}"

# 使用 hdiutil 创建自包含的 DMG
DMG_STAGING="${PKG_DIR}/dmg_staging"
rm -rf "${DMG_STAGING}"
mkdir -p "${DMG_STAGING}"
cp -R "${APP_PATH}" "${DMG_STAGING}/"
ln -s /Applications "${DMG_STAGING}/Applications"

hdiutil create -volname "ShapefileViewer" -srcfolder "${DMG_STAGING}" -ov -format UDZO "${FINAL_DMG}"

echo "=== 8. 验证 DMG 完整性 ==="
hdiutil verify "${FINAL_DMG}"
echo "macOS DMG 打包成功！产物位于: ${FINAL_DMG}"
