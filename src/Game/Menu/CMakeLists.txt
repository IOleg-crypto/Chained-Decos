add_library(menu STATIC
    Menu.cpp
    Menu.h
)

target_include_directories(menu PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/Engine
)

target_link_libraries(menu PUBLIC ChainedDecosEngine raylib imguilib nlohmann_json::nlohmann_json config)