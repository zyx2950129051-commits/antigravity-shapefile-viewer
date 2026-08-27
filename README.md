# SHP轻量查看器 (ShpLightViewer)

一个基于 C++17、CMake 与 Qt 6 开发的现代化、极速轻量、跨平台 ESRI Shapefile (*.shp) 桌面查看器。

---

## 🌟 核心特性

- **跨平台原生支持**：
  - **macOS**：提供 Apple Silicon 原生独立安装镜像 DMG（`ShpLightViewer-0.1.0-macOS-arm64.dmg`）。
  - **Windows**：提供自包含全动态库的 Windows 10/11 x64 NSIS 安装向导（`ShpLightViewer-0.1.0-Windows-x64-Setup.exe`）。
- **静态嵌入 Shapelib 1.6.3**：
  - 通过 CMake `FetchContent` 静态编译集成 [OSGeo/shapelib 1.6.3](https://github.com/OSGeo/shapelib)，无需用户在系统中安装额外的 GIS 动态库。
- **全几何类型与孔洞镂空支持**：
  - 点 (`Point`/`PointZ`/`PointM`) 与多点 (`MultiPoint`)
  - 折线 (`Polyline`) 与多部件折线 (`Multi-part Polyline`)
  - 多边形 (`Polygon`) 及带内环孔洞的多边形（采用 `OddEvenFill` 规则自动镂空）
  - 自动过滤 `Null Shape`，支持 Z/M 维度自动降维至平面 X/Y。
- **📊 现代化要素属性表**：
  - 自动检测并读取同名 `.dbf` 属性文件与 `.cpg` 编码文件；
  - 采用 `QAbstractTableModel` 虚拟表格架构，万级数据秒级流畅加载与实时模糊搜索过滤；
  - 完美支持中文字符属性，彻底杜绝乱码。
- **🎯 地图要素点击拾取与双向联动**：
  - 鼠标左键点击地图上的点、线或多边形要素，该要素将以醒目的亮橙色/金色粗轮廓高亮呈现；
  - 点击地图要素自动滚动定位属性表对应行；在属性表中选中某行，地图自动居中并高亮该要素。
- **🗑️ 选中要素一键删除**：
  - 支持在地图或属性表中选中要素后一键删除（支持 `Delete / Backspace` 快捷键），自动重新计算图层总要素数、总顶点数与空间包围盒。
- **高性能交互画布**：
  - 视区裁剪（Viewport Culling）：快速剔除可视范围外的要素，中大型数据缩放平移极速响应；
  - 滚轮以鼠标光标所在地理位置为锚点进行 $1.2\times$ 平滑缩放；
  - 鼠标左键拖拽平移，支持一键“适应窗口”。

---

## 🛠️ 构建与测试

### 环境要求
- **CMake**: >= 3.20
- **C++ 编译器**: 支持 C++17（Clang 15+ / GCC 10+ / MSVC 2022）
- **Qt 6**: >= 6.5 (需要 `Widgets`, `Concurrent`, `Test` 组件)

### macOS 本地构建与打包
```bash
# 1. 编译与运行测试
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$(brew --prefix qt)"
cmake --build build --config Release -j$(sysctl -n hw.ncpu)
ctest --test-dir build --output-on-failure

# 2. 打包 macOS DMG
./scripts/build_macos.sh
```

### Windows 本地构建与打包 (MSVC 2022 x64)
```powershell
# 1. 配置 CMake 与编译
cmake -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="C:/Qt/6.8.2/msvc2022_64"
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure

# 2. 收集依赖并打包 NSIS
windeployqt --release --no-translations --compiler-runtime "build/Release/ShpLightViewer.exe"
cd build
cpack -G NSIS -C Release
```

---

## 📦 交付产物

构建产物统一归档至 `outputs/` 目录：
- `outputs/ShpLightViewer-0.1.0-Windows-x64-Setup.exe`：Windows 10/11 x64 NSIS 独立安装向导。
- `outputs/ShpLightViewer-0.1.0-macOS-arm64.dmg`：macOS Apple Silicon 独立 DMG 安装镜像。
- `outputs/ShpLightViewer-0.1.0-Source.zip`：项目完整跨平台源码包。
- `outputs/ShpLightViewer-0.1.0-checksums.txt`：各产物的 SHA-256 校验和。

---

## 📄 开源许可证

本项目源码基于 MIT 许可证分发。
- **Shapelib**: 遵循 LGPL 2.0 / MIT 开源许可证。
- **Qt 6**: 遵循 LGPLv3 / 商业许可证。
