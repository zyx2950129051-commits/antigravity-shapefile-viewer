cmake_minimum_required(VERSION 3.20)

project(ShapefileViewer
    VERSION 0.1.0
    DESCRIPTION "现代化轻量级 C++ Shapefile 桌面查看器"
    LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find required Qt 6 packages
find_package(Qt6 6.5 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    Concurrent
)

# Fetch and build Shapelib 1.6.3 statically
include(cmake/Shapelib.cmake)

# Application Sources
set(CORE_SOURCES
    src/core/ShapeData.h
    src/core/ShapeData.cpp
    src/core/ShapefileReader.h
    src/core/ShapefileReader.cpp
)

set(UI_SOURCES
    src/ui/ShapeCanvas.h
    src/ui/ShapeCanvas.cpp
    src/ui/MainWindow.h
    src/ui/MainWindow.cpp
)

add_executable(ShapefileViewer
    src/main.cpp
    ${CORE_SOURCES}
    ${UI_SOURCES}
)

target_include_directories(ShapefileViewer PRIVATE
    src/core
    src/ui
)

target_link_libraries(ShapefileViewer PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Concurrent
    shapelib
)

# Platform specific configurations
if(APPLE)
    set_target_properties(ShapefileViewer PROPERTIES
        MACOSX_BUNDLE ON
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.geoviewer.shapefileviewer"
        MACOSX_BUNDLE_BUNDLE_NAME "ShapefileViewer"
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
    )
elseif(WIN32)
    set_target_properties(ShapefileViewer PROPERTIES
        WIN32_EXECUTABLE ON
    )
endif()

# Installation rules
include(GNUInstallDirs)
install(TARGETS ShapefileViewer
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(WIN32)
    # Bundle Qt DLLs and plugins created by windeployqt into the installer
    install(DIRECTORY "${CMAKE_BINARY_DIR}/bin/Release/"
        DESTINATION ${CMAKE_INSTALL_BINDIR}
        FILES_MATCHING
        PATTERN "*.dll"
        PATTERN "platforms/*"
        PATTERN "styles/*"
        PATTERN "imageformats/*"
        PATTERN "tls/*"
        PATTERN "iconengines/*"
    )
    install(DIRECTORY "${CMAKE_BINARY_DIR}/Release/"
        DESTINATION ${CMAKE_INSTALL_BINDIR}
        FILES_MATCHING
        PATTERN "*.dll"
        PATTERN "platforms/*"
        PATTERN "styles/*"
        PATTERN "imageformats/*"
        PATTERN "tls/*"
        PATTERN "iconengines/*"
    )
endif()

# Enable testing
enable_testing()
add_subdirectory(tests)

# Packaging via CPack (NSIS for Windows, DragNDrop for macOS)
set(CPACK_PACKAGE_NAME "ShapefileViewer")
set(CPACK_PACKAGE_VENDOR "GeoViewer")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "ESRI Shapefile Desktop Viewer")

if(WIN32)
    set(CPACK_GENERATOR "NSIS")
    set(CPACK_NSIS_DISPLAY_NAME "Shapefile 查看器")
    set(CPACK_NSIS_PACKAGE_NAME "ShapefileViewer")
    set(CPACK_NSIS_MODIFY_PATH OFF)
    set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
    set(CPACK_NSIS_CREATE_ICONS_EXTRA
        "CreateShortCut '$DESKTOP\\\\Shapefile 查看器.lnk' '$INSTDIR\\\\bin\\\\ShapefileViewer.exe'"
    )
    set(CPACK_NSIS_DELETE_ICONS_EXTRA
        "Delete '$DESKTOP\\\\Shapefile 查看器.lnk'"
    )
elseif(APPLE)
    set(CPACK_GENERATOR "DragNDrop")
endif()

include(CPack)
