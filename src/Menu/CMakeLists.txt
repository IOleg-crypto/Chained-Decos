add_library(menu STATIC
    Menu.cpp
    Menu.h
)
target_include_directories(menu PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src/Menu)
target_link_libraries(menu PUBLIC raylib imguilib) 