add_library(cameracontroller STATIC
    Core/CameraController.cpp
    Core/CameraController.h
    Interfaces/ICameraSensitivityController.h
)

target_include_directories(cameracontroller
    PUBLIC
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/scene/3d/camera
        ${CMAKE_SOURCE_DIR}/include/imgui
)

target_link_libraries(cameracontroller PUBLIC raylib)

set_target_properties(cameracontroller PROPERTIES POSITION_INDEPENDENT_CODE ON)