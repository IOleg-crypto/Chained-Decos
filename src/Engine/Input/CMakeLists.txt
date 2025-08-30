add_library(input STATIC
    InputManager.cpp
    InputManager.h
)
target_link_libraries(input PUBLIC raylib) 