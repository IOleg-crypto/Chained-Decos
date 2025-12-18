set(MAP_SOURCES
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

add_library(map STATIC ${MAP_SOURCES})

target_include_directories(map
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/include
)


target_link_libraries(map PUBLIC raylib nlohmann_json config render)

set_target_properties(map PROPERTIES POSITION_INDEPENDENT_CODE ON)

