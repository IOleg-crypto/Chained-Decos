add_library(menu STATIC
    Menu.cpp
    Menu.h
    MenuCore.cpp
    MenuCore.h
    SettingsManager.cpp
    SettingsManager.h
    ConsoleManager.cpp
    ConsoleManager.h
    MapSelector.cpp
    MapSelector.h
    MenuConstants.h
)

target_include_directories(menu PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/Engine
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/rlImGui
    ${CMAKE_SOURCE_DIR}/include/imgui
)

target_link_libraries(menu PUBLIC
    ChainedDecosEngine
    ChainedDecosGame
    raylib
    imguilib
    nlohmann_json::nlohmann_json
    config
    mapmod
)