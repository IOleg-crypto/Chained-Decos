cmake_minimum_required(VERSION 3.20)

# Add subdirectories
add_subdirectory(config)
add_subdirectory(object/event)
add_subdirectory(object/module)

# Core library - базові системи
add_library(core STATIC
    # Math
    math/Matrix/Matrix4x4.cpp
    math/Vector/Vector3.cpp
)

target_include_directories(core
    PUBLIC
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/core
)

target_link_libraries(core
    PUBLIC
        nlohmann_json
        raylib
        config
        event
        module
)
