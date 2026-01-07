add_library(audio_core STATIC
    Audio.cpp
    Audio.h
)

target_include_directories(audio_core PUBLIC
    .
    ${PROJECT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/engine
)

target_link_libraries(audio_core PUBLIC
    core
    raylib
)
