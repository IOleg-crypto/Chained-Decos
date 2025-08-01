add_library(input STATIC
    InputManager.cpp
    InputManager.h
)
target_include_directories(input PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src/Input)
target_link_libraries(input PUBLIC raylib) 