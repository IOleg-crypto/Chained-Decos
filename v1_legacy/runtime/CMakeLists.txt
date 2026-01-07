set(GAME_SOURCES
    runtime_layer.cpp
    runtime_layer.h
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
        ${CMAKE_CURRENT_SOURCE_DIR}/logic
)

# PROJECT_ROOT_DIR is defined globally in the root CMakeLists.txt


target_link_libraries(RuntimeLib
    PUBLIC
        raylib
        core
        engine
        events
        project
)

# Standalone Runtime Executable
add_executable(Runtime 
    runtime_main.cpp
    runtime_application.cpp 
    runtime_application.h
)

target_link_libraries(Runtime
    PRIVATE
        RuntimeLib
)

set_target_properties(Runtime PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
