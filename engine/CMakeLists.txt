set(ENGINE_SOURCES
    application.cpp
    layer_stack.cpp
    scene.cpp
    renderer.cpp
    asset_manager.cpp
    scene_serializer.cpp
    project.cpp
    project_serializer.cpp
    physics.cpp
)

add_library(engine STATIC ${ENGINE_SOURCES})
target_link_libraries(engine PUBLIC yaml-cpp)

target_link_libraries(engine PUBLIC raylib imguilib EnTT)
target_include_directories(engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
