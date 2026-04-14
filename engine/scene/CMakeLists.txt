add_library(engine_scene ${SUBMODULE_LIB_TYPE}
        yaml.h
        scene.cpp
        project.cpp
        project_serializer.cpp
        scene_serializer.cpp
        component_serializer.cpp
        hierarchy_serializer.cpp
        prefab_serializer.cpp
        scene_camera.cpp
        editor_camera.cpp
        entity.cpp
        scene_picking.cpp
        components/mesh_component.cpp

        scene.h
        scene_events.h
        project.h
        project_serializer.h
        scene_serializer.h
        component_serializer.h
        hierarchy_serializer.h
        prefab_serializer.h
        scene_camera.h
        editor_camera.h
        scene_picking.h
        scene_settings.h
        components.h
        components/animation_component.h
        components/audio_component.h
        components/camera_component.h
        components/control_component.h
        components/game_components.h
        components/hierarchy_component.h
        components/id_component.h
        components/light_component.h
        components/mesh_component.h
        components/navigation_component.h
        components/physics_component.h
        components/scripting_components.h
        components/shader_component.h
        components/tag_component.h
        components/transform_component.h
        components/sprite_component.h
        entity.h
)

target_include_directories(engine_scene PUBLIC
    ${ENGINE_INCLUDE_DIRS}
    ${yaml-cpp_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/imgui
    ${CMAKE_SOURCE_DIR}/include/imgui/backends
    ${CMAKE_SOURCE_DIR}/include/entt/src
)

target_link_libraries(engine_scene PUBLIC 
    engine_core
    engine_graphics
    engine_physics
    engine_audio
    yaml-cpp
    EnTT::EnTT
)

if(COMMAND apply_engine_optimizations)
    apply_engine_optimizations(engine_scene)
endif()
