add_library(cameracontroller STATIC
    CameraController.cpp
    CameraController.h
)
target_link_libraries(cameracontroller PUBLIC raylib) 