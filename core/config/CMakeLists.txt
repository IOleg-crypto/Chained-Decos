
set(CONFIGMANAGER_SOURCE 
    config_manager.cpp
    config_manager.h
)
set(GAMECONFIG_SOURCE 
    game_config.cpp
    game_config.h
)

add_library(config STATIC
    ${CONFIGMANAGER_SOURCE}
    ${GAMECONFIG_SOURCE}
)
target_include_directories(config
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
)

target_link_libraries(config
    PUBLIC
    raylib
)

set_target_properties(config PROPERTIES POSITION_INDEPENDENT_CODE ON)