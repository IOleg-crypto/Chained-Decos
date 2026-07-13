# Header-only module: pure API type declarations shared by engine_assets and engine_graphics.
# The implementations (*.cpp) live in engine_graphics (which compiles api/*.cpp via GLOB_RECURSE).
add_library(engine_graphics_api INTERFACE)

target_include_directories(engine_graphics_api INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
)

target_link_libraries(engine_graphics_api INTERFACE
    engine_core
    glm
    cereal
)
