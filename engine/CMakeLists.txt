set(ENGINE_SOURCES
    core/application.cpp
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
    renderer/buffer.cpp
    renderer/vertex_array.cpp
    renderer/shader.cpp
    renderer/render_api.cpp
    renderer/opengl_buffer.cpp
    renderer/opengl_vertex_array.cpp
    renderer/opengl_shader.cpp
    renderer/opengl_render_api.cpp
    physics/physics.cpp
    physics/physics.h
    physics/bvh/bvh.cpp
    physics/bvh/bvh.h
    audio/audio_manager.cpp
    audio/audio_manager.h
    ../game/src/game_scripts.cpp
)

add_library(engine STATIC ${ENGINE_SOURCES})
target_link_libraries(engine PUBLIC yaml-cpp)

target_link_libraries(engine PUBLIC raylib imguilib EnTT)
target_include_directories(engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Copy engine resources to the build directory
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/resources" DESTINATION "${CMAKE_BINARY_DIR}/engine")
