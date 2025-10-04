# Performance CMakeLists.txt

# Collect source files
file(GLOB PERFORMANCE_SOURCES
    "*.cpp"
)

file(GLOB PERFORMANCE_HEADERS
    "*.h"
)

# Create library
add_library(PerformanceManager STATIC
    ${PERFORMANCE_SOURCES}
    ${PERFORMANCE_HEADERS}
)

# Include directories
target_include_directories(PerformanceManager PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
)

# Link dependencies
target_link_libraries(PerformanceManager PRIVATE
    raylib
)

# Compiler definitions
target_compile_definitions(PerformanceManager PRIVATE
    PROJECT_ROOT_DIR="${CMAKE_SOURCE_DIR}"
)

# Organize in IDE
source_group("Source Files" FILES ${PERFORMANCE_SOURCES})
source_group("Header Files" FILES ${PERFORMANCE_HEADERS})

# Output messages
message(STATUS "PerformanceManager: Added ${PERFORMANCE_SOURCES}")