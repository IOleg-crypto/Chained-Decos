cmake_minimum_required(VERSION 3.20)

# 3D subdirectories
add_subdirectory(camera)

# Add imgui for camera controller
target_include_directories(cameracontroller PUBLIC ${CMAKE_SOURCE_DIR}/include/imgui)
