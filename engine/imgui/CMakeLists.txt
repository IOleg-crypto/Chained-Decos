add_library(engine_imgui STATIC
    imgui_layer.h
    imgui_layer.cpp
)

target_include_directories(engine_imgui PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
)

target_link_libraries(engine_imgui PUBLIC
    engine_core
    engine_external_imgui
    engine_platform
)

if(TARGET engine_pch)
    target_link_libraries(engine_imgui PRIVATE engine_pch)
endif()
