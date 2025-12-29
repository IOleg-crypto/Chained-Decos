set(GAME_SOURCES
    RuntimeLayer.cpp
    RuntimeLayer.h
    logic/RuntimeInitializer.cpp
    logic/RuntimeInitializer.h
)

add_library(RuntimeLib STATIC ${GAME_SOURCES})

target_include_directories(RuntimeLib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/runtime
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/components/physics/collision
        ${CMAKE_SOURCE_DIR}/scene/resources/model
        ${CMAKE_SOURCE_DIR}/scene/resources/map
        ${CMAKE_SOURCE_DIR}/components/rendering
        ${CMAKE_SOURCE_DIR}/include/imgui
        ${CMAKE_SOURCE_DIR}/include/rlImGui
        ${CMAKE_SOURCE_DIR}/core
        ${CMAKE_SOURCE_DIR}/scene/camera/core
        ${CMAKE_SOURCE_DIR}/scene/main
        ${CMAKE_SOURCE_DIR}/components/physics/dynamics/src
        ${CMAKE_CURRENT_SOURCE_DIR}/logic
)

# PROJECT_ROOT_DIR is defined globally in the root CMakeLists.txt


target_link_libraries(RuntimeLib
    PUBLIC
        world
        raylib
        imguilib
        nlohmann_json
        core
        scene_manager
        cameracontroller
        model
        map
        ecs
        render
        collision
        audio
        input
        events
        font_resources
        texture_resources
)

# Standalone Runtime Executable
add_executable(Runtime 
    RuntimeMain.cpp
    RuntimeApplication.cpp 
    RuntimeApplication.h
)

target_link_libraries(Runtime
    PRIVATE
        RuntimeLib
)

set_target_properties(Runtime PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
