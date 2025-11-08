add_library(map STATIC
    MapLoader.cpp
    MapLoader.h
    MapData.h
    Skybox/Skybox.cpp
    Skybox/Skybox.h
)

target_include_directories(map
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/include
)

# Add PROJECT_ROOT_DIR definition for consistent path handling
target_compile_definitions(map PRIVATE
    PROJECT_ROOT_DIR="${CMAKE_SOURCE_DIR}"
)

# Use forward slashes for cross-platform compatibility
file(TO_CMAKE_PATH "${CMAKE_SOURCE_DIR}" PROJECT_ROOT_DIR_ESCAPED)
target_compile_definitions(map PRIVATE
    PROJECT_ROOT_DIR="${PROJECT_ROOT_DIR_ESCAPED}"
)

target_link_libraries(map PUBLIC raylib nlohmann_json)

set_target_properties(map PROPERTIES POSITION_INDEPENDENT_CODE ON)

