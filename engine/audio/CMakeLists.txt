add_library(engine_audio STATIC
    audio.h
    audio.cpp
)

target_include_directories(engine_audio PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
)

target_link_libraries(engine_audio PUBLIC
    engine_core
    engine_project
    glm
    engine_external_miniaudio
)

if(WIN32)
    target_link_libraries(engine_audio PUBLIC winmm ole32)
endif()

if(TARGET engine_pch)
    target_link_libraries(engine_audio PRIVATE engine_pch)
endif()
