add_library(world STATIC
    Physics.h
    Physics.cpp

) 
target_link_libraries(world PUBLIC raylib collision player)