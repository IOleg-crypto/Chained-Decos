set(CAMERA_SOURCES
    core/CameraController.h
    core/CameraController.cpp
    core/ICameraSensitivityController.h
)

add_library(cameracontroller STATIC ${CAMERA_SOURCES})
target_include_directories(cameracontroller PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/core)
target_link_libraries(cameracontroller PUBLIC raylib imguilib core)



