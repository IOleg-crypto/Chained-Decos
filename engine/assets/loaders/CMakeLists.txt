add_library(engine_graphics_loaders STATIC
    asset_loader.h
    assimp_importer.h
    assimp_importer.cpp
    audio_loader.h
    audio_loader.cpp
    environment_loader.h
    environment_loader.cpp
    font_loader.h
    font_loader.cpp
    material_loader.h
    material_loader.cpp
    model_loader.h
    model_loader.cpp
    shader_loader.h
    shader_loader.cpp
    stb_image.cpp
    texture_loader.h
    texture_loader.cpp
)

target_include_directories(engine_graphics_loaders PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
)

target_link_libraries(engine_graphics_loaders PUBLIC
    engine_graphics
)

if(TARGET engine_pch)
    target_link_libraries(engine_graphics_loaders PRIVATE engine_pch)
endif()
