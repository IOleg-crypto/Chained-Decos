chained_add_engine_module(engine_scene
    DEPENDS
        engine_reflection
        engine_core
        engine_graphics
        engine_project
        engine_physics
        engine_audio
        engine_scripting
        engine_external_entt
        glm
        engine_external_imgui
)

set_target_properties(engine_scene PROPERTIES UNITY_BUILD OFF)
