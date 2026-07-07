chained_add_engine_module(engine_platform_sdl3
    SOURCES
        sdl_window.cpp
        sdl_window.h
    DEPENDS
        engine_core
        engine_external_glad
        engine_external_imgui
)

target_link_libraries(engine_platform_sdl3 PRIVATE SDL3-static)
target_include_directories(engine_platform_sdl3 PRIVATE
    "${CMAKE_SOURCE_DIR}/thirdparty/SDL/include"
    "${CMAKE_SOURCE_DIR}/thirdparty/stb"
)
target_compile_definitions(engine_platform_sdl3 PRIVATE CH_PLATFORM_BACKEND_SDL3)
