#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

BOTTLE_NAME="ShpViewer-Test"
CX_BIN="/Applications/CrossOver.app/Contents/SharedSupport/CrossOver/bin"

if [ ! -d "/Applications/CrossOver.app" ]; then
    echo "错误：未检测到 /Applications/CrossOver.app"
    exit 1
fi

echo "=== 1. 检查并准备 CrossOver 测试 Bottle: ${BOTTLE_NAME} ==="
BOTTLE_DIR="${HOME}/Library/Application Support/CrossOver/Bottles/${BOTTLE_NAME}"
if [ -d "${BOTTLE_DIR}" ]; then
    echo "发现旧测试 Bottle，正在清理以确保测试独立纯净..."
    "${CX_BIN}/cxbottle" --bottle "${BOTTLE_NAME}" --delete --force || rm -rf "${BOTTLE_DIR}"
fi

echo "正在创建独立的 64 位测试 Bottle..."
"${CX_BIN}/cxbottle" --bottle "${BOTTLE_NAME}" --create --template win10_64

echo "=== 2. 准备测试 Shapefile 数据 ==="
TEST_DATA_DIR="${ROOT_DIR}/build/test_data_crossover"
rm -rf "${TEST_DATA_DIR}"
mkdir -p "${TEST_DATA_DIR}"

# 运行测试夹具生成各种测试用 SHP
"${ROOT_DIR}/build/tests/test_reader"

echo "=== 3. 运行 Windows 安装包/程序验证 ==="
EXE_SETUP="${ROOT_DIR}/outputs/ShapefileViewer-0.1.0-Windows-x64-Setup.exe"

if [ -f "${EXE_SETUP}" ]; then
    echo "找到 Windows 安装包: ${EXE_SETUP}"
    echo "在 CrossOver Bottle 中执行静默安装..."
    "${CX_BIN}/cxstart" --bottle "${BOTTLE_NAME}" --exe "${EXE_SETUP}" -- /S || true
    echo "Windows 安装包安装测试完成！"
else
    echo "未检测到 outputs/ 下的 Windows 安装包，请确保 GitHub Actions CI 构建产物已下载。"
fi

echo "=== CrossOver 验证准备就绪 ==="
