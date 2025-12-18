cmake_minimum_required(VERSION 3.20)

# Add subdirectories
add_subdirectory(config)
add_subdirectory(object/event)
add_subdirectory(object/kernel)
add_subdirectory(object/module)
add_subdirectory(ecs)

# Core library - base systems
set(CORE_SOURCES
    engine/Engine.cpp
    engine/Engine.h
    engine/EngineApplication.cpp
    engine/EngineApplication.h
    engine/IApplication.h
    services/CoreServices.cpp
    services/CoreServices.h
)

add_library(core STATIC ${CORE_SOURCES})

target_include_directories(core
    PUBLIC
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/core
        ${CMAKE_SOURCE_DIR}/include/raylib/src/external/glfw/include
)

target_link_libraries(core
    PUBLIC
        nlohmann_json
        raylib
        EnTT::EnTT
        config
        event
        kernel
        module
        ecs
        audio
        input
        render
        collision
        model
)
