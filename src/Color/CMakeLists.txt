add_library(color STATIC
    ColorParser.h
)
target_include_directories(color PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src/Color)
target_link_libraries(color PUBLIC raylib) 