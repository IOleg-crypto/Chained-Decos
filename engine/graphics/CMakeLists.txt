set(STB_IMPL_SOURCE "${CMAKE_CURRENT_BINARY_DIR}/stb_impl.cpp")
file(WRITE "${STB_IMPL_SOURCE}" "#define STB_IMAGE_IMPLEMENTATION\n#include \"stb_image.h\"\n")

chained_add_engine_module(engine_graphics
    SOURCES
        ${STB_IMPL_SOURCE}
    EXCLUDE_DIRS
        loaders
    DEPENDS
        engine_core
        engine_project
        engine_audio
        engine_assets
        yaml-cpp
        engine_external_entt
        glm
        engine_external_stb
        engine_external_imgui
        engine_external_glad
        assimp
)

# Prevents multiple definition errors when using unity builds
set_source_files_properties(${STB_IMPL_SOURCE} PROPERTIES SKIP_UNITY_BUILD_INCLUSION ON)
