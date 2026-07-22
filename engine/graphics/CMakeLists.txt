
chained_add_engine_module(engine_graphics
    SOURCES
        api
        pipeline
        ui
    DEPENDS
        engine_core
        engine_project
        engine_assets
        yaml-cpp
        engine_external_entt
        glm
        engine_external_stb
        engine_external_imgui
        engine_external_glad
        assimp
)
