set(ENGINE_SOURCES
    core/application.cpp
    core/window.cpp
    core/thread_dispatcher.cpp
    core/process_utils.cpp
    core/uuid.cpp
    core/input.cpp
    core/layer_stack.cpp
    scene/scene.cpp
    scene/scene_serializer.cpp
    scene/project.cpp
    scene/project_serializer.cpp
    renderer/render.cpp
    renderer/render_command.cpp
    renderer/scene_render.cpp
    renderer/asset_manager.cpp
    renderer/model_asset.cpp
    renderer/texture_asset.cpp
    renderer/shader_asset.cpp
    audio/sound_asset.cpp
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
target_include_directories(engine PUBLIC ${CMAKE_SOURCE_DIR}/include/json/single_include)

# Copy engine resources to the build directory
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/resources" DESTINATION "${CMAKE_BINARY_DIR}/engine")
