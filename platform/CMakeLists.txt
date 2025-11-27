cmake_minimum_required(VERSION 3.20)

add_library(ChainedDecosEngine STATIC
    windows/Core/Engine.cpp
    windows/Core/EngineApplication.cpp
)

target_include_directories(ChainedDecosEngine PUBLIC
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/platform
)

target_link_libraries(ChainedDecosEngine PUBLIC
    core
    render
    input
    audio
    raylib
    imguilib
)
