# Define the unified engine target
add_library(engine STATIC)

# Common Engine Properties
target_include_directories(engine PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
)

target_sources(engine PRIVATE
    # --- CORE ---
    ${CMAKE_CURRENT_SOURCE_DIR}/core/application.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/application.h
    ${CMAKE_CURRENT_SOURCE_DIR}/core/base.h
    ${CMAKE_CURRENT_SOURCE_DIR}/core/entry_point.h
    ${CMAKE_CURRENT_SOURCE_DIR}/core/events.h
    ${CMAKE_CURRENT_SOURCE_DIR}/core/input.h
    ${CMAKE_CURRENT_SOURCE_DIR}/core/layer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/core/layer_stack.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/layer_stack.h
    ${CMAKE_CURRENT_SOURCE_DIR}/core/log.h
    ${CMAKE_CURRENT_SOURCE_DIR}/core/profiler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/profiler.h
    ${CMAKE_CURRENT_SOURCE_DIR}/core/uuid.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/uuid.h
    ${CMAKE_CURRENT_SOURCE_DIR}/core/window.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/window.h
    ${CMAKE_CURRENT_SOURCE_DIR}/core/yaml.h

    # --- GRAPHICS ---
    ${CMAKE_CURRENT_SOURCE_DIR}/graphics/api_context.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/graphics/api_context.h
    ${CMAKE_CURRENT_SOURCE_DIR}/graphics/asset.h
    ${CMAKE_CURRENT_SOURCE_DIR}/graphics/asset_manager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/graphics/asset_manager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/graphics/draw_command.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/graphics/draw_command.h
    ${CMAKE_CURRENT_SOURCE_DIR}/graphics/environment.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/graphics/environment.h
    ${CMAKE_CURRENT_SOURCE_DIR}/graphics/font_asset.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/graphics/font_asset.h
    ${CMAKE_CURRENT_SOURCE_DIR}/graphics/material.h
    ${CMAKE_CURRENT_SOURCE_DIR}/graphics/model_asset.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/graphics/model_asset.h
    ${CMAKE_CURRENT_SOURCE_DIR}/graphics/render_types.h
    ${CMAKE_CURRENT_SOURCE_DIR}/graphics/scene_pipeline.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/graphics/scene_pipeline.h
    ${CMAKE_CURRENT_SOURCE_DIR}/graphics/shader_asset.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/graphics/shader_asset.h
    ${CMAKE_CURRENT_SOURCE_DIR}/graphics/texture_asset.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/graphics/texture_asset.h
    ${CMAKE_CURRENT_SOURCE_DIR}/graphics/visuals.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/graphics/visuals.h

    # --- SCENE ---
    ${CMAKE_CURRENT_SOURCE_DIR}/scene/scene.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/scene/scene.h
    ${CMAKE_CURRENT_SOURCE_DIR}/scene/entity.h
    ${CMAKE_CURRENT_SOURCE_DIR}/scene/project.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/scene/project.h
    ${CMAKE_CURRENT_SOURCE_DIR}/scene/project_serializer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/scene/project_serializer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/scene/scene_serializer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/scene/scene_serializer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/scene/component_serializer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/scene/scene_scripting.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/scene/scene_scripting.h
    ${CMAKE_CURRENT_SOURCE_DIR}/scene/script_registry.h
    ${CMAKE_CURRENT_SOURCE_DIR}/scene/scriptable_entity.h
    
    # Components
    ${CMAKE_CURRENT_SOURCE_DIR}/scene/components/audio_component.h
    ${CMAKE_CURRENT_SOURCE_DIR}/scene/components/camera_component.h
    ${CMAKE_CURRENT_SOURCE_DIR}/scene/components/game_components.h
    ${CMAKE_CURRENT_SOURCE_DIR}/scene/components/hierarchy_component.h
    ${CMAKE_CURRENT_SOURCE_DIR}/scene/components/id_component.h
    ${CMAKE_CURRENT_SOURCE_DIR}/scene/components/lighting_component.h
    ${CMAKE_CURRENT_SOURCE_DIR}/scene/components/mesh_component.h
    ${CMAKE_CURRENT_SOURCE_DIR}/scene/components/physics_component.h
    ${CMAKE_CURRENT_SOURCE_DIR}/scene/components/scripting_components.h
    ${CMAKE_CURRENT_SOURCE_DIR}/scene/components/tag_component.h
    ${CMAKE_CURRENT_SOURCE_DIR}/scene/components/transform_component.h
    ${CMAKE_CURRENT_SOURCE_DIR}/scene/components/control_component.h
    ${CMAKE_CURRENT_SOURCE_DIR}/scene/components/animation_component.h

    # --- PHYSICS ---
    ${CMAKE_CURRENT_SOURCE_DIR}/physics/physics.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/physics/physics.h
    ${CMAKE_CURRENT_SOURCE_DIR}/physics/dynamics.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/physics/dynamics.h
    ${CMAKE_CURRENT_SOURCE_DIR}/physics/narrow_phase.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/physics/narrow_phase.h
    ${CMAKE_CURRENT_SOURCE_DIR}/physics/scene_trace.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/physics/scene_trace.h
    ${CMAKE_CURRENT_SOURCE_DIR}/physics/collision/collision.h
    ${CMAKE_CURRENT_SOURCE_DIR}/physics/collision/collision_triangle.h
    ${CMAKE_CURRENT_SOURCE_DIR}/physics/bvh/bvh.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/physics/bvh/bvh.h
    ${CMAKE_CURRENT_SOURCE_DIR}/physics/bvh/bvh_node.h

    # --- AUDIO ---
    ${CMAKE_CURRENT_SOURCE_DIR}/audio/sound_asset.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/audio/sound_asset.h
)

# External Dependencies (Propagated through engine)
target_link_libraries(engine PUBLIC 
    raylib
    yaml-cpp
    EnTT
    imguilib
    glm
)

if(MSVC)
    target_compile_options(engine PRIVATE /bigobj)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    # For GCC/MinGW, large projects might need this
    target_compile_options(engine PRIVATE -W)
endif()

# Apply Unity Build & PCH
apply_engine_optimizations(engine)

# Copy engine resources to the build directory
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/resources" DESTINATION "${CMAKE_BINARY_DIR}/engine")
