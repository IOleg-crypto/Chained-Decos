add_library(audio STATIC
    AudioManager.cpp
    AudioManager.h
)

target_include_directories(audio PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src/Engine
)

target_link_libraries(audio PUBLIC raylib)