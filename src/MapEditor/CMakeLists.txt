cmake_minimum_required(VERSION 3.31)
project(ChainedDecosMapEditor)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(${PROJECT_NAME} Application.cpp Map.cpp Map.h)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src/
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    engine
    player
    model
    menu
    input
    mapmod
    cameracontroller
    color
    world
    raylib
    nlohmann_json
    imguilib
)
