add_library(menu STATIC
    Menu.cpp
    Menu.h
    MenuController/MenuController.cpp
    MenuController/MenuController.h
    Settings/SettingsManager.cpp
    Settings/SettingsManager.h
    Console/ConsoleManager.cpp
    Console/ConsoleManager.h
    Console/ConsoleManagerHelpers.cpp
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