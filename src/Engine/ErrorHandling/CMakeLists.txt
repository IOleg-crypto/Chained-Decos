# ErrorHandling CMakeLists.txt

# Collect source files
file(GLOB ERROR_HANDLING_SOURCES
    "*.cpp"
)

file(GLOB ERROR_HANDLING_HEADERS
    "*.h"
)

# Create library
add_library(ErrorHandling STATIC
    ${ERROR_HANDLING_SOURCES}
    ${ERROR_HANDLING_HEADERS}
)

# Include directories
target_include_directories(ErrorHandling PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
)

# Link dependencies
target_link_libraries(ErrorHandling PRIVATE
    raylib
)

# Compiler definitions
target_compile_definitions(ErrorHandling PRIVATE
    PROJECT_ROOT_DIR="${CMAKE_SOURCE_DIR}"
)

# Use forward slashes for cross-platform compatibility
file(TO_CMAKE_PATH "${CMAKE_SOURCE_DIR}" PROJECT_ROOT_DIR_ESCAPED)
target_compile_definitions(ErrorHandling PRIVATE
    PROJECT_ROOT_DIR="${PROJECT_ROOT_DIR_ESCAPED}"
)

# Organize in IDE
source_group("Source Files" FILES ${ERROR_HANDLING_SOURCES})
source_group("Header Files" FILES ${ERROR_HANDLING_HEADERS})

# Output messages
message(STATUS "ErrorHandling: Added ${ERROR_HANDLING_SOURCES}")