# Define the Game module as a DLL
add_library(ChainedDecosGame SHARED)

# Game Sources
target_sources(ChainedDecosGame PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game_module.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/playercontroller.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cameracontroller.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/exitscript.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/playergui.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/scenescript.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/spawnzone.h
)

target_link_libraries(ChainedDecosGame PUBLIC engine)
target_compile_definitions(ChainedDecosGame PRIVATE GAME_BUILD_DLL)

# Disable static runtime for MinGW to avoid conflicts with engine.dll
if(MINGW)
    target_link_options(ChainedDecosGame PRIVATE -shared)
    # Remove the default -static-libgcc and -static-libstdc++
    string(REPLACE "-static-libgcc" "" CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS}")
    string(REPLACE "-static-libstdc++" "" CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS}")
endif()

apply_engine_optimizations(ChainedDecosGame)

target_include_directories(ChainedDecosGame PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}
)

# Copy the DLL to the runtime output directory
add_custom_command(TARGET ChainedDecosGame POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:ChainedDecosGame>
    ${CMAKE_BINARY_DIR}/bin/ChainedDecosGame.dll
)

# Installation
install(TARGETS ChainedDecosGame
    ARCHIVE DESTINATION lib COMPONENT Runtime
    LIBRARY DESTINATION lib COMPONENT Runtime
    RUNTIME DESTINATION bin COMPONENT Runtime
)
