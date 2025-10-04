add_library(event STATIC
    EventSystem.cpp
    EventSystem.h
)

target_include_directories(event PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src/Engine
)