add_library(cameracontroller STATIC
    CameraController.cpp
    CameraController.h
)
target_include_directories(cameracontroller PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src/CameraController)
target_link_libraries(cameracontroller PUBLIC raylib) 