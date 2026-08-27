find_package(Qt6 REQUIRED COMPONENTS Core Widgets Concurrent Test)

include_directories(
    ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/src/ui
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Test 1: Shapefile Reader & Data Model
add_executable(test_reader
    TestReader.cpp
    TestDataGenerator.cpp
    ${CMAKE_SOURCE_DIR}/src/core/ShapeData.cpp
    ${CMAKE_SOURCE_DIR}/src/core/ShapefileReader.cpp
)
target_link_libraries(test_reader PRIVATE
    Qt6::Core
    Qt6::Test
    shapelib
)
add_test(NAME test_reader COMMAND test_reader)

# Test 2: Canvas Transform Math & Viewport Logic
add_executable(test_canvas_transform
    TestCanvasTransform.cpp
    ${CMAKE_SOURCE_DIR}/src/core/ShapeData.cpp
)
target_link_libraries(test_canvas_transform PRIVATE
    Qt6::Core
    Qt6::Test
)
add_test(NAME test_canvas_transform COMMAND test_canvas_transform)

# Test 3: Offscreen Smoke Test for UI & Async Loading
add_executable(test_smoke
    TestSmoke.cpp
    TestDataGenerator.cpp
    ${CMAKE_SOURCE_DIR}/src/core/ShapeData.cpp
    ${CMAKE_SOURCE_DIR}/src/core/ShapefileReader.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/ShapeCanvas.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/AttributeTableDialog.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/MainWindow.cpp
)
target_link_libraries(test_smoke PRIVATE
    Qt6::Widgets
    Qt6::Concurrent
    Qt6::Test
    shapelib
)
add_test(NAME test_smoke COMMAND test_smoke)

# Test 4: DBF Attribute Table & Feature Selection / Deletion
add_executable(test_attributes
    TestAttributes.cpp
    TestDataGenerator.cpp
    ${CMAKE_SOURCE_DIR}/src/core/ShapeData.cpp
    ${CMAKE_SOURCE_DIR}/src/core/ShapefileReader.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/ShapeCanvas.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/AttributeTableDialog.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/MainWindow.cpp
)
target_link_libraries(test_attributes PRIVATE
    Qt6::Widgets
    Qt6::Concurrent
    Qt6::Test
    shapelib
)
add_test(NAME test_attributes COMMAND test_attributes)
