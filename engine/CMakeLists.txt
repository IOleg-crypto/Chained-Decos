set(ENGINE_SOURCES
    application.cpp
    layer_stack.cpp
    scene.cpp
    renderer.cpp
)

add_library(engine STATIC ${ENGINE_SOURCES})

target_link_libraries(engine PUBLIC raylib imguilib EnTT)
target_include_directories(engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
