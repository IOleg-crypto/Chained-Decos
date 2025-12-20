set(MAP_SOURCES
    core/MapLoader.cpp
    core/MapLoader.h
    core/MapData.h
    core/MapService.cpp
    core/MapService.h
    converter/MapObjectConverter.cpp
    converter/MapObjectConverter.h
    interfaces/IMapFileOperations.h
    interfaces/IMapMetadataManager.h
    renderer/MapRenderer.cpp
    renderer/MapRenderer.h
    skybox/skybox.cpp
    skybox/skybox.h
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



