add_library(map STATIC
    Core/MapLoader.cpp
    Core/MapLoader.h
    Core/MapData.h
    Core/MapService.cpp
    Core/MapService.h
    Converter/MapObjectConverter.cpp
    Converter/MapObjectConverter.h
    Interfaces/IMapFileOperations.h
    Interfaces/IMapMetadataManager.h
    Renderer/MapRenderer.cpp
    Renderer/MapRenderer.h
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

target_link_libraries(map PUBLIC raylib nlohmann_json config)

set_target_properties(map PROPERTIES POSITION_INDEPENDENT_CODE ON)

