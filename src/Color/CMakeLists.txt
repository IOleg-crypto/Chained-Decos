add_library(color STATIC
    ColorParser.h
    ColorParser.cpp
)
target_link_libraries(color PUBLIC raylib) 