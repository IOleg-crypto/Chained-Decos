add_library(menu STATIC
    Menu.cpp
    Menu.h
)
target_link_libraries(menu PUBLIC raylib imguilib) 