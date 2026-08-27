# Shapefile 查看器 (ShapefileViewer)

一个基于 C++17、CMake 与 Qt 6 开发的现代化、轻量级、跨平台 ESRI Shapefile (*.shp) 桌面查看器。

---

## 🌟 核心特性

- **跨平台支持**：支持 Apple Silicon macOS（提供独立 DMG 安装镜像）与 Windows 10/11 x64（提供 NSIS 安装包 EXE）。
- **静态嵌入 Shapelib 1.6.3**：通过 CMake `FetchContent` 静态编译集成 [OSGeo/shapelib 1.6.3](https://github.com/OSGeo/shapelib)，无需用户在系统中安装额外的 GIS 动态库。
- **全要素二维几何支持**：
  - 点 (`Point`, `PointZ`, `PointM`) 与多点 (`MultiPoint`, `MultiPointZ`, `MultiPointM`)
  - 折线 (`Polyline`, `PolylineZ`, `PolylineM`) 及多部件折线 (`Multi-part Polyline`)
  - 多边形 (`Polygon`, `PolygonZ`, `PolygonM`) 及带内环孔洞的多边形（采用 `OddEvenFill` 规则自动镂空）
  - 自动过滤 `Null Shape`，支持 Z/M 维度自动降维至平面 X/Y。
- **高性能交互式画布**：
  - 自动居中与 24 像素边距自适应，Y 轴自动翻转对齐地理坐标系。
  - 视区裁剪（Viewport Culling）：快速剔除屏幕可视范围外的要素，中小型数据缩放平移极速响应。
  - 滚轮以鼠标光标所在地理位置为锚点进行 $1.2\times$ 平滑缩放（缩放范围约束为基准比例的 $0.1\times \sim 1000\times$）。
  - 鼠标左键拖拽平移，支持一键“适应窗口”。
- **异步防假死与容错机制**：
  - 基于 `QtConcurrent` 后台异步解析大型 Shapefile，界面不卡顿。
  - 失败保护机制：加载损坏或非法文件时弹出友好中文提示，且**保留当前已显示的图层**不被冲掉。
- **原生中文路径与国际化支持**：
  - 完美支持包含中文字符的目录路径与文件名。

---

## 🛠️ 构建与测试

### 环境要求
- **CMake**: >= 3.20
- **C++ 编译器**: 支持 C++17（Clang 15+ / GCC 10+ / MSVC 2022）
- **Qt 6**: >= 6.5 (需要 `Widgets`, `Concurrent`, `Test` 组件)

### macOS 本地构建
```bash
# 1. 配置 CMake
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="/opt/homebrew/opt/qt"

# 2. 编译
cmake --build build --config Release -j$(sysctl -n hw.ncpu)

# 3. 运行单元测试
ctest --test-dir build --output-on-failure

# 4. 打包 DMG
./scripts/build_macos.sh
```

### Windows 本地构建 (MSVC 2022 x64)
```powershell
# 1. 配置 CMake
cmake -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="C:/Qt/6.11.1/msvc2022_64"

# 2. 编译与测试
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure

# 3. 收集依赖并打包 NSIS
windeployqt --release --no-translations --compiler-runtime "build/Release/ShapefileViewer.exe"
cd build
cpack -G NSIS -C Release
```

---

## 📦 交付产物

构建产物统一归档至 `outputs/` 目录：
- `outputs/ShapefileViewer-0.1.0-macOS-arm64.dmg`：macOS Apple Silicon 独立 DMG 安装镜像。
- `outputs/ShapefileViewer-0.1.0-Windows-x64-Setup.exe`：Windows 10/11 x64 NSIS 安装向导。
- `outputs/ShapefileViewer-0.1.0-checksums.txt`：各产物的 SHA-256 校验和。

---

## 📄 开源许可证

本项目源码基于 MIT 许可证分发。
内嵌的第三方库：
- **Shapelib**: 遵循 LGPL 2.0 / MIT 开源许可证。
- **Qt 6**: 遵循 LGPLv3 / 商业许可证。
