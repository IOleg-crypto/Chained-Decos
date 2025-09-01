add_library(menu STATIC
    Menu.cpp
    Menu.h
)

target_include_directories(menu PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(menu PUBLIC raylib imguilib nlohmann_json::nlohmann_json)