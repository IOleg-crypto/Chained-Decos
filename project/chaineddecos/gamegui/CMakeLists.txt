set(MENU_SOURCES
    Menu.cpp
    Menu.h
    Settings/SettingsManager.cpp
    Settings/SettingsManager.h
    Settings/MenuSettingsController.cpp
    Settings/MenuSettingsController.h
    Console/ConsoleManager.cpp
    Console/ConsoleManager.h
    MapSelector/MapSelector.cpp
    MapSelector/MapSelector.h
    MenuConstants.h
    screens/BaseMenuScreen.cpp
    screens/BaseMenuScreen.h
    screens/MainMenuScreen.cpp
    screens/MainMenuScreen.h
    screens/MapSelectionScreen.cpp
    screens/MapSelectionScreen.h
    screens/OptionsScreen.cpp
    screens/OptionsScreen.h
    screens/CreditsScreen.cpp
    screens/CreditsScreen.h
    screens/ConfirmExitScreen.cpp
    screens/ConfirmExitScreen.h
    screens/SettingsScreens.cpp
    screens/SettingsScreens.h
)

add_library(menu STATIC ${MENU_SOURCES})

target_include_directories(menu PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/project/chaineddecos/gamegui
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/rlImGui
    ${CMAKE_SOURCE_DIR}/include/imgui
    ${CMAKE_SOURCE_DIR}/include/raylib/src/external/glfw/include
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
    raylib
    imguilib
    nlohmann_json::nlohmann_json
    cameracontroller
)