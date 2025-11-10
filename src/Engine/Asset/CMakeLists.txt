# Asset CMakeLists.txt

# Collect source files
file(GLOB ASSET_SOURCES
    "Core/*.cpp"
)

file(GLOB ASSET_HEADERS
    "Core/*.h"
)

# Create library
add_library(AdvancedAssetManager STATIC
    ${ASSET_SOURCES}
    ${ASSET_HEADERS}
)

# Include directories
target_include_directories(AdvancedAssetManager PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
)

# Link dependencies
target_link_libraries(AdvancedAssetManager PRIVATE
    raylib
    ErrorHandling
)

set_target_properties(AdvancedAssetManager PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Compiler definitions
target_compile_definitions(AdvancedAssetManager PRIVATE
    PROJECT_ROOT_DIR="${CMAKE_SOURCE_DIR}"
)

# Use forward slashes for cross-platform compatibility
file(TO_CMAKE_PATH "${CMAKE_SOURCE_DIR}" PROJECT_ROOT_DIR_ESCAPED)
target_compile_definitions(AdvancedAssetManager PRIVATE
    PROJECT_ROOT_DIR="${PROJECT_ROOT_DIR_ESCAPED}"
)

# Organize in IDE
source_group("Source Files" FILES ${ASSET_SOURCES})
source_group("Header Files" FILES ${ASSET_HEADERS})

# Output messages
message(STATUS "AdvancedAssetManager: Added ${ASSET_SOURCES}")