chained_add_engine_module(engine_scene
    SOURCES
        component_serializer.cpp
        component_serializer.h
        hierarchy_serializer.cpp
        hierarchy_serializer.h
        prefab_serializer.cpp
        prefab_serializer.h
        scene_serializer.cpp
        scene_serializer.h
        scripting_serialization.h
        serialization.h
        yaml.h
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
