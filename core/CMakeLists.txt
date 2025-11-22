cmake_minimum_required(VERSION 3.20)

# Core library - базові системи
add_library(core STATIC
    # Math
    math/Matrix/Matrix4x4.cpp
    math/Vector/Vector3.cpp
    
    # Config
    config/Core/ConfigManager.cpp
    
    # Event System
    object/event/Core/EventDispatcher.cpp
    object/event/Core/EventSystem.cpp
    
    # Kernel
    object/kernel/Core/Kernel.cpp
    
    # Module System
    object/module/Core/ModuleManager.cpp
)

target_include_directories(core
    PUBLIC
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/core
)

target_link_libraries(core
    PUBLIC
        nlohmann_json
)
