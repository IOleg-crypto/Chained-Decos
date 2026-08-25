add_library(engine_platform_glfw STATIC
    glfw_window.h
    glfw_window.cpp
    glfw_input_mapper.h
)

target_include_directories(engine_platform_glfw PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
)

target_link_libraries(engine_platform_glfw PUBLIC
    engine_core
    glfw
    engine_external_glad
    engine_external_imgui
)

target_compile_definitions(engine_platform_glfw PRIVATE GLFW_INCLUDE_NONE CH_PLATFORM_BACKEND_GLFW)

if(TARGET engine_pch)
    target_link_libraries(engine_platform_glfw PRIVATE engine_pch)
endif()
