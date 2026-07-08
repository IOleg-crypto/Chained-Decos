
add_library(editor_core STATIC)

# Find all source files automatically (Lazy/Convenience mode)
file(GLOB_RECURSE EDITOR_SOURCES CONFIGURE_DEPENDS "*.cpp" "*.h")
list(FILTER EDITOR_SOURCES EXCLUDE REGEX ".*/main.cpp$")

target_sources(editor_core PRIVATE ${EDITOR_SOURCES})


target_link_libraries(editor_core PUBLIC
    ChainedEngine::Framework
    engine_external_imgui
    nfd
    assimp
    Coral.Native
    engine_core
)

target_include_directories(editor_core PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Define the Editor executable
add_executable(ChainedEditor ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
        icons.h)
target_link_libraries(ChainedEditor PRIVATE editor_core ChainedDecos)
target_include_directories(ChainedEditor PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
)

# Apply optimizations
if(COMMAND apply_engine_optimizations)
    apply_engine_optimizations(editor_core)
    apply_engine_optimizations(ChainedEditor)
endif()

# Automatically sync resources/ into the correct bin/<Config>/ directory after each build
if(COMMAND ch_add_resource_sync)
    ch_add_resource_sync(ChainedEditor)
endif()

# Installation
install(TARGETS ChainedEditor editor_core
    RUNTIME DESTINATION bin COMPONENT Runtime
    ARCHIVE DESTINATION lib COMPONENT Runtime
    LIBRARY DESTINATION lib COMPONENT Runtime
)

# Copy global assets to the install directory if they exist
if(EXISTS "${CMAKE_SOURCE_DIR}/assets")
    install(DIRECTORY "${CMAKE_SOURCE_DIR}/assets/" DESTINATION assets COMPONENT Runtime)
endif()
