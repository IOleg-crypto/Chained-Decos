set(INPUT_SOURCES
    core/InputManager.cpp
    core/InputManager.h
)

add_library(input STATIC ${INPUT_SOURCES})

target_include_directories(input PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src/Engine 
    ${CMAKE_SOURCE_DIR}/src 
)

target_link_libraries(input PUBLIC 
    raylib
    events
)

target_compile_definitions(input PRIVATE CHAINEDDECOSENGINE_EXPORTS)

set_target_properties(input PROPERTIES POSITION_INDEPENDENT_CODE ON)

