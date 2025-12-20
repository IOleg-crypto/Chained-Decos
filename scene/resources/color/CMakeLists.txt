set(COLOR_SOURCES
    ColorParser.cpp
    ColorParser.h
)

add_library(color STATIC ${COLOR_SOURCES})

target_include_directories(color PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(color PUBLIC
    raylib
)

set_target_properties(color PROPERTIES POSITION_INDEPENDENT_CODE ON)
