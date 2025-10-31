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

# Add PROJECT_ROOT_DIR definition for consistent path handling
target_compile_definitions(menu PRIVATE
    PROJECT_ROOT_DIR="${CMAKE_SOURCE_DIR}"
)

# Use forward slashes for cross-platform compatibility
file(TO_CMAKE_PATH "${CMAKE_SOURCE_DIR}" PROJECT_ROOT_DIR_ESCAPED)
target_compile_definitions(menu PRIVATE
    PROJECT_ROOT_DIR="${PROJECT_ROOT_DIR_ESCAPED}"
)

target_link_libraries(menu PUBLIC
    ChainedDecosEngine
    raylib
    imguilib
    nlohmann_json::nlohmann_json
    config
)