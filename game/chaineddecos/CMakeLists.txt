# Define the Game module as a Standalone Executable
chained_add_game(ChainedDecosGame
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game_module.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/playercontroller.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cameracontroller.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/exitscript.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/playergui.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/scenescript.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/spawnzone.h
)

target_include_directories(ChainedDecosGame PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}
)

# Installation
install(TARGETS ChainedDecosGame ChainedDecosGameExe
    RUNTIME DESTINATION bin COMPONENT Runtime
    ARCHIVE DESTINATION lib COMPONENT Runtime
    LIBRARY DESTINATION lib COMPONENT Runtime
)
