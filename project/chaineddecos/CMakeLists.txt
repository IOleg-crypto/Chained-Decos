set(GAME_SOURCES
    GameLayer.cpp
    GameLayer.h
    logic/GameInitializer.cpp
    logic/GameInitializer.h
)

add_library(ChainedDecosGame STATIC ${GAME_SOURCES})

target_include_directories(ChainedDecosGame
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/project/ChainedDecos
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/project/ChainedDecos/player
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

add_subdirectory(gamegui)
add_subdirectory(player)


target_link_libraries(ChainedDecosGame
    PUBLIC
        player
        menu
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
)

# Standalone Runtime Executable
add_executable(ChainedDecos 
    RuntimeMain.cpp
    GameApplication.cpp 
    GameApplication.h
)

target_link_libraries(ChainedDecos
    PRIVATE
        ChainedDecosGame
)

set_target_properties(ChainedDecos PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
