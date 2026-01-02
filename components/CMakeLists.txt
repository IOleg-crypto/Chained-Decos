cmake_minimum_required(VERSION 3.20)

# Servers subdirectories
add_subdirectory(rendering)
add_subdirectory(physics)
add_subdirectory(audio)

# Servers interface library
add_library(servers INTERFACE)

target_link_libraries(servers
    INTERFACE
        rendering
        physics_collision
        physics_dynamics
        audio
)
