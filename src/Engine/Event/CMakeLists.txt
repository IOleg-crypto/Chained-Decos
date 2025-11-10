add_library(event STATIC
    Core/EventSystem.cpp
    Core/EventSystem.h
)

target_include_directories(event PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src/Engine
)

set_target_properties(event PROPERTIES POSITION_INDEPENDENT_CODE ON)