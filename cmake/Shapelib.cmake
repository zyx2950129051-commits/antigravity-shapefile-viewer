include(FetchContent)

FetchContent_Declare(
    shapelib
    GIT_REPOSITORY https://github.com/OSGeo/shapelib.git
    GIT_TAG        v1.6.3
    GIT_SHALLOW    TRUE
)

# Avoid building shapelib tests/executables if shapelib cmake defines them
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(BUILD_APPS OFF CACHE BOOL "" FORCE)

FetchContent_GetProperties(shapelib)
if(NOT shapelib_POPULATED)
    FetchContent_Populate(shapelib)
    
    # We create a clean, portable static library target for shapelib
    add_library(shapelib STATIC
        "${shapelib_SOURCE_DIR}/shpopen.c"
        "${shapelib_SOURCE_DIR}/shptree.c"
        "${shapelib_SOURCE_DIR}/dbfopen.c"
        "${shapelib_SOURCE_DIR}/safileio.c"
    )
    
    target_include_directories(shapelib PUBLIC
        "${shapelib_SOURCE_DIR}"
    )
    
    # Position Independent Code for static library integration
    set_target_properties(shapelib PROPERTIES
        POSITION_INDEPENDENT_CODE ON
        C_STANDARD 99
    )
    
    if(WIN32)
        target_compile_definitions(shapelib PRIVATE _CRT_SECURE_NO_WARNINGS)
    endif()
endif()
