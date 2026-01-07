cmake_minimum_required(VERSION 3.20)

set(CORE_SOURCES
    Log.h
    utils/base.h
    utils/types.h
    utils/core.h
)

add_library(core STATIC ${CORE_SOURCES})

target_include_directories(core
    PUBLIC
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/core
        ${CMAKE_SOURCE_DIR}/include/raylib/src/external/glfw/include
        ${CMAKE_SOURCE_DIR}/include/raylib/src
        ${CMAKE_SOURCE_DIR}/include/raylib/src/external
)

target_link_libraries(core
    PUBLIC
        nlohmann_json
        raylib
        events
)

target_compile_definitions(core
    PRIVATE
        _CRT_SECURE_NO_WARNINGS
)
