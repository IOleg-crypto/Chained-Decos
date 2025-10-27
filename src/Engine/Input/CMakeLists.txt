add_library(input STATIC
    InputManager.cpp
    InputManager.h
)

target_include_directories(input PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR} # Дозволяє іншим модулям включати заголовки Input
)

target_link_libraries(input PUBLIC raylib)

target_compile_definitions(input PRIVATE CHAINEDDECOSENGINE_EXPORTS)

set_target_properties(input PROPERTIES POSITION_INDEPENDENT_CODE ON)