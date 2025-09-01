cmake_minimum_required(VERSION 3.20)
project(ChainedDecosEngine)

add_subdirectory(Collision)
add_subdirectory(Input)
add_subdirectory(Menu)
add_subdirectory(Model)
add_subdirectory(Render)
add_subdirectory(CameraController)

add_library(ChainedDecosEngine INTERFACE
        Engine.h
        Engine.cpp)

target_include_directories(ChainedDecosEngine INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/src/World

)

target_link_libraries(ChainedDecosEngine INTERFACE
    raylib
    imguilib
    Collision
    Input
    Menu
    Model
    Render
    CameraController
)