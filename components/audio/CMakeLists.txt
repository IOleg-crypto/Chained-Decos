set(AUDIO_SOURCES
    core/AudioManager.cpp
    core/AudioManager.h
    interfaces/IAudioManager.h
)

add_library(audio STATIC ${AUDIO_SOURCES})

target_include_directories(audio PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src/Engine
)

target_link_libraries(audio PUBLIC raylib)

set_target_properties(audio PROPERTIES POSITION_INDEPENDENT_CODE ON)

