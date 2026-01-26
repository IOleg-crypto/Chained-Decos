# Define the unified engine target
add_library(engine STATIC)

# Add subdirectories to register their sources
add_subdirectory(core)
add_subdirectory(render)
add_subdirectory(scene)
add_subdirectory(physics)
add_subdirectory(audio)
add_subdirectory(ui)

# Common Engine Properties
target_include_directories(engine PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
)

# External Dependencies (Propagated through engine)
target_link_libraries(engine PUBLIC 
    raylib
    yaml-cpp
    EnTT
    imguilib
)

# Apply Unity Build & PCH
apply_engine_optimizations(engine)

# Copy engine resources to the build directory
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/resources" DESTINATION "${CMAKE_BINARY_DIR}/engine")
