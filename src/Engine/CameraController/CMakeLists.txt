add_library(cameracontroller STATIC
    CameraController.cpp
    CameraController.h
)

target_include_directories(cameracontroller PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(cameracontroller PUBLIC raylib)

set_target_properties(cameracontroller PROPERTIES POSITION_INDEPENDENT_CODE ON)