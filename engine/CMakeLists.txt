# Define the unified engine target
add_library(engine STATIC)

# Add subdirectories to register their sources
add_subdirectory(core)
add_subdirectory(graphics)
add_subdirectory(scene)
add_subdirectory(physics)
add_subdirectory(audio)

# Common Engine Properties
target_include_directories(engine PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
)

target_sources(engine PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/core/application.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/uuid.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/window.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/layer_stack.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/profiler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/graphics/draw_command.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/graphics/environment.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/graphics/shader_asset.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/graphics/asset_manager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/graphics/texture_asset.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/graphics/model_asset.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/graphics/font_asset.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/audio/sound_asset.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/physics/physics.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/physics/dynamics.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/physics/narrow_phase.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/physics/scene_trace.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/physics/bvh/bvh.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/scene/scene.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/scene/project.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/scene/project_serializer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/scene/scene_serializer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/scene/component_serializer.cpp
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
