set(MAP_SOURCES
    SceneLoader.cpp
    SceneLoader.h
    GameScene.cpp
    GameScene.h
    MapData.h
    MapService.cpp
    MapService.h
    SceneSerializer.cpp
    SceneSerializer.h
    MapObjectConverter.cpp
    MapObjectConverter.h
    ISceneFileOperations.h
    IMapMetadataManager.h
    MapRenderer.cpp
    MapRenderer.h
    Skybox.cpp
    skybox.h
)

add_library(map STATIC ${MAP_SOURCES})

target_include_directories(map
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/include/json/include
)


target_link_libraries(map PUBLIC raylib nlohmann_json config render)

set_target_properties(map PROPERTIES POSITION_INDEPENDENT_CODE ON)



