set(ENGINE_SOURCES
    core/application.cpp
    core/input.cpp
    core/layer_stack.cpp
    core/thread_pool.cpp
    scene/scene.cpp
    scene/scene_serializer.cpp
    scene/project.cpp
    scene/project_serializer.cpp
    renderer/renderer.cpp
    renderer/asset_manager.cpp
    physics/physics.cpp
    physics/physics.h
    physics/bvh/bvh.cpp
    physics/bvh/bvh.h
    audio/audio_manager.cpp
    audio/audio_manager.h
)

add_library(engine STATIC ${ENGINE_SOURCES})
target_link_libraries(engine PUBLIC yaml-cpp)

target_link_libraries(engine PUBLIC raylib imguilib EnTT)
target_include_directories(engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
