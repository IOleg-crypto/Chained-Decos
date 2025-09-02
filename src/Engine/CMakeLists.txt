cmake_minimum_required(VERSION 3.20)
project(ChainedDecosEngine LANGUAGES CXX C)

add_subdirectory(Collision)
add_subdirectory(Input)
add_subdirectory(Model)
add_subdirectory(Render)
add_subdirectory(CameraController)
add_subdirectory(World)

add_library(ChainedDecosEngine STATIC
        Engine.cpp
        Engine.h
)
target_include_directories(ChainedDecosEngine
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)
target_compile_features(ChainedDecosEngine PUBLIC cxx_std_20)
target_link_libraries(ChainedDecosEngine
        PUBLIC
        collision
        input
        model
        render
        cameracontroller
        world
        PRIVATE
        raylib
        imguilib
)
