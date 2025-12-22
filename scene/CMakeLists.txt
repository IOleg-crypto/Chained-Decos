cmake_minimum_required(VERSION 3.20)

# Scene subdirectories
add_subdirectory(main)
add_subdirectory(resources)
add_subdirectory(gui)
add_subdirectory(camera)

# Scene Manager Library
add_library(scene_manager STATIC SceneManager.cpp SceneManager.h)
target_link_libraries(scene_manager PUBLIC raylib map)
target_include_directories(scene_manager PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Add imgui include path for all scene components
include_directories(${CMAKE_SOURCE_DIR}/include/imgui)
