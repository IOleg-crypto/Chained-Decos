set(CAMERA_SOURCES
    CameraController.h
    CameraController.cpp
    ICameraSensitivityController.h
)

add_library(cameracontroller STATIC ${CAMERA_SOURCES})
target_include_directories(cameracontroller PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(cameracontroller PUBLIC raylib imguilib core)



