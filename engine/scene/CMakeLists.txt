# Register Scene sources to the unified engine target
target_sources(engine PRIVATE
    # Scene
    ${CMAKE_CURRENT_SOURCE_DIR}/scene.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/scene.h
    ${CMAKE_CURRENT_SOURCE_DIR}/entity.h
    ${CMAKE_CURRENT_SOURCE_DIR}/components.h
    ${CMAKE_CURRENT_SOURCE_DIR}/scene_serializer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/scene_serializer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/project.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/project.h
    ${CMAKE_CURRENT_SOURCE_DIR}/project_serializer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/project_serializer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/script_registry.h
    ${CMAKE_CURRENT_SOURCE_DIR}/scriptable_entity.h
    ${CMAKE_CURRENT_SOURCE_DIR}/control.h
    ${CMAKE_CURRENT_SOURCE_DIR}/control_base.h
    ${CMAKE_CURRENT_SOURCE_DIR}/controls.h

    # Components (Subfolders)
    ${CMAKE_CURRENT_SOURCE_DIR}/components/audio_component.h
    ${CMAKE_CURRENT_SOURCE_DIR}/components/camera_component.h
    ${CMAKE_CURRENT_SOURCE_DIR}/components/game_components.h
    ${CMAKE_CURRENT_SOURCE_DIR}/components/hierarchy_component.h
    ${CMAKE_CURRENT_SOURCE_DIR}/components/id_component.h
    ${CMAKE_CURRENT_SOURCE_DIR}/components/lighting_component.h
    ${CMAKE_CURRENT_SOURCE_DIR}/components/mesh_component.h
    ${CMAKE_CURRENT_SOURCE_DIR}/components/physics_component.h
    ${CMAKE_CURRENT_SOURCE_DIR}/components/scripting_components.h
    ${CMAKE_CURRENT_SOURCE_DIR}/components/tag_component.h
    ${CMAKE_CURRENT_SOURCE_DIR}/components/transform_component.h
    ${CMAKE_CURRENT_SOURCE_DIR}/components/control_component.h
    ${CMAKE_CURRENT_SOURCE_DIR}/components/animation_component.h
)
