cmake_minimum_required(VERSION 3.20)
project(ShpLightViewer VERSION 0.1.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable Qt AUTOMOC, AUTOUIC, AUTORCC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Find Qt 6 packages
find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    Concurrent
)

# Fetch and build OSGeo/shapelib statically
include(cmake/Shapelib.cmake)

# Source files
set(CORE_SOURCES
    src/core/ShapeData.h
    src/core/ShapeData.cpp
    src/core/ShapefileReader.h
    src/core/ShapefileReader.cpp
)

set(UI_SOURCES
    src/ui/ShapeCanvas.h
    src/ui/ShapeCanvas.cpp
    src/ui/AttributeTableDialog.h
    src/ui/AttributeTableDialog.cpp
    src/ui/MainWindow.h
    src/ui/MainWindow.cpp
)

add_executable(ShpLightViewer
    src/main.cpp
    ${CORE_SOURCES}
    ${UI_SOURCES}
)

target_include_directories(ShpLightViewer PRIVATE
    src/core
    src/ui
)

target_link_libraries(ShpLightViewer PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Concurrent
    shapelib
)

# Platform specific configurations
if(APPLE)
    set_target_properties(ShpLightViewer PROPERTIES
        MACOSX_BUNDLE ON
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.geoviewer.shplightviewer"
        MACOSX_BUNDLE_BUNDLE_NAME "SHP轻量查看器"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
    )
elseif(WIN32)
    set_target_properties(ShpLightViewer PROPERTIES
        WIN32_EXECUTABLE ON
    )
endif()

# Installation rules
include(GNUInstallDirs)
install(TARGETS ShpLightViewer
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
set(CPACK_PACKAGE_NAME "ShpLightViewer")
set(CPACK_PACKAGE_VENDOR "GeoViewer")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "SHP轻量查看器 (ShpLightViewer)")

if(WIN32)
    set(CPACK_GENERATOR "NSIS")
    set(CPACK_NSIS_DISPLAY_NAME "SHP轻量查看器")
    set(CPACK_NSIS_PACKAGE_NAME "ShpLightViewer")
    set(CPACK_NSIS_MODIFY_PATH OFF)
    set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
    set(CPACK_NSIS_CREATE_ICONS_EXTRA
        "CreateShortCut '$DESKTOP\\\\SHP轻量查看器.lnk' '$INSTDIR\\\\bin\\\\ShpLightViewer.exe'"
    )
    set(CPACK_NSIS_DELETE_ICONS_EXTRA
        "Delete '$DESKTOP\\\\SHP轻量查看器.lnk'"
    )
elseif(APPLE)
    set(CPACK_GENERATOR "DragNDrop")
endif()

include(CPack)
