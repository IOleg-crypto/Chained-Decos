cmake_minimum_required(VERSION 3.20)

# Scene subdirectories
add_subdirectory(main)
add_subdirectory(resources)
add_subdirectory(gui)
add_subdirectory(camera)
add_subdirectory(ecs)

# Scene Core Library (Hazel-style Scene/Entity)
add_library(scene_core STATIC 
    core/scene.cpp 
    core/scene.h 
    core/entity.cpp 
    core/entity.h
    core/scene_serializer.cpp
    core/scene_serializer.h
    core/scene_manager.cpp
    core/scene_manager.h
)
target_link_libraries(scene_core PUBLIC 
    raylib 
    EnTT::EnTT 
    yaml-cpp
    nlohmann_json::nlohmann_json
)
target_include_directories(scene_core PUBLIC 
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include/json/include
    ${CMAKE_CURRENT_SOURCE_DIR}
)


# Add imgui include path for all scene components
include_directories(${CMAKE_SOURCE_DIR}/include/imgui)
