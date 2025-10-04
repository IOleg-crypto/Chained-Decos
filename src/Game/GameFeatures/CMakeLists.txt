# GameFeatures CMakeLists.txt

# Collect source files
file(GLOB GAME_FEATURES_SOURCES
    "*.cpp"
)

file(GLOB GAME_FEATURES_HEADERS
    "*.h"
)

# Create library
add_library(GameFeatures STATIC
    ${GAME_FEATURES_SOURCES}
    ${GAME_FEATURES_HEADERS}
)

# Include directories
target_include_directories(GameFeatures PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
)

# Link dependencies
target_link_libraries(GameFeatures PRIVATE
    raylib
)

# Compiler definitions
target_compile_definitions(GameFeatures PRIVATE
    PROJECT_ROOT_DIR="${CMAKE_SOURCE_DIR}"
)

# Organize in IDE
source_group("Source Files" FILES ${GAME_FEATURES_SOURCES})
source_group("Header Files" FILES ${GAME_FEATURES_HEADERS})

# Output messages
message(STATUS "GameFeatures: Added ${GAME_FEATURES_SOURCES}")