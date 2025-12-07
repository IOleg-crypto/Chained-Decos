add_library(menu STATIC
    Menu.cpp
    Menu.h
    MenuPresenter.cpp
    MenuPresenter.h
    Settings/SettingsManager.cpp
    Settings/SettingsManager.h
    Settings/MenuSettingsController.cpp
    Settings/MenuSettingsController.h
    Console/ConsoleManager.cpp
    Console/ConsoleManager.h
    MapSelector/MapSelector.cpp
    MapSelector/MapSelector.h
    MenuConstants.h
)

target_include_directories(menu PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/project/chained_decos/Menu
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
)