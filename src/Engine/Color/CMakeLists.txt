add_library(color STATIC
    ColorParser.h
    ColorParser.cpp
)

target_include_directories(color PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(color PUBLIC raylib)

set_target_properties(color PROPERTIES POSITION_INDEPENDENT_CODE ON)