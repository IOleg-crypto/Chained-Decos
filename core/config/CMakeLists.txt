add_library(config STATIC
    Core/ConfigManager.cpp
    Core/ConfigManager.h
    Core/GameConfig.cpp
    Core/GameConfig.h
)

target_include_directories(config
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(config
    PUBLIC
    raylib
)

set_target_properties(config PROPERTIES POSITION_INDEPENDENT_CODE ON)