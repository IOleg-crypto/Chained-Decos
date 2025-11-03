
add_subdirectory(Collision)
add_subdirectory(Config)
add_subdirectory(Input)
add_subdirectory(Model)
add_subdirectory(Map)
add_subdirectory(Render)
add_subdirectory(CameraController)
add_subdirectory(Physics)
add_subdirectory(World)
add_subdirectory(CommandLineHandler)
add_subdirectory(Kernel)
add_subdirectory(Module)
add_subdirectory(Audio)
add_subdirectory(Event)

add_library(ChainedDecosEngine STATIC
          Engine.cpp
          Engine.h
          MapFileManager/MapFileManager.cpp
          MapFileManager/JsonMapFileManager.cpp
          Application/EngineApplication.cpp
          Application/EngineApplication.h
)
target_include_directories(ChainedDecosEngine
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(ChainedDecosEngine
            PUBLIC
            collision
            config
            input
            model
            map
            render
            cameracontroller
            physics
            world
            kernel
            module
            audio
            event
            raylib
            imguilib
            cmdhandler
    )

target_compile_definitions(ChainedDecosEngine PRIVATE CHAINEDDECOSENGINE_EXPORTS)

set_target_properties(ChainedDecosEngine PROPERTIES POSITION_INDEPENDENT_CODE ON)
