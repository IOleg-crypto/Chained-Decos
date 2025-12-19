cmake_minimum_required(VERSION 3.20)

# Scene subdirectories
add_subdirectory(main)
add_subdirectory(resources)
add_subdirectory(gui)
add_subdirectory(camera)

# Add imgui include path for all scene components
include_directories(${CMAKE_SOURCE_DIR}/include/imgui)
