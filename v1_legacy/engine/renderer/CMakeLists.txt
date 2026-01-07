add_library(renderer_core STATIC
    Renderer.cpp
    Renderer.h
    renderer_api.cpp
    renderer_api.h
    Buffer.cpp
    Buffer.h
    vertex_array.cpp
    vertex_array.h
    Shader.cpp
    Shader.h
    opengl_renderer_api.cpp
    opengl_renderer_api.h
    opengl_buffer.cpp
    opengl_buffer.h
    opengl_vertex_array.cpp
    opengl_vertex_array.h
    opengl_shader.cpp
    opengl_shader.h
)

target_include_directories(renderer_core PUBLIC
    .
    ${PROJECT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/engine
)

target_link_libraries(renderer_core PUBLIC
    core
    raylib
    events
)
