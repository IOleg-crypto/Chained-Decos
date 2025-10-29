cmake_minimum_required(VERSION 3.25)

# MapEditor project - depends on ChainedDecosEngine
project(ChainedDecosMapEditor VERSION 1.0.0)

# Raylib is provided by parent project
# No need to find_package here as it's already available

# MapEditor source files
set(MAPEDITOR_SOURCES
    main.cpp
    Application.cpp
    Editor/Editor.cpp
    Editor/Object/MapObject.cpp
    Editor/FileManager/FileManager.cpp
    Editor/FileManager/IFileManager.h
    Editor/FileManager/FileManager.h
    Editor/SceneManager/SceneManager.cpp
    Editor/SceneManager/SceneManager.h
    Editor/SceneManager/ISceneManager.h
    Editor/UIManager/UIManager.cpp
    Editor/UIManager/UIManager.h
    Editor/UIManager/IUIManager.h
    Editor/ToolManager/ToolManager.cpp
    Editor/ToolManager/ToolManager.h
    Editor/ToolManager/IToolManager.h
    Editor/CameraManager/CameraManager.cpp
    Editor/CameraManager/CameraManager.h
    Editor/CameraManager/ICameraManager.h
    Editor/ModelManager/ModelManager.cpp
    Editor/ModelManager/ModelManager.h
    Editor/ModelManager/IModelManager.h
)


# Combine all sources
set(ALL_SOURCES ${MAPEDITOR_SOURCES})

# Create MapEditor executable
add_executable(ChainedDecosMapEditor ${ALL_SOURCES})

# Include directories
target_include_directories(ChainedDecosMapEditor PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/nfd
)

# Link with ChainedDecosEngine, raylib, and mapmod for map loading functionality
target_link_libraries(ChainedDecosMapEditor PRIVATE ChainedDecosEngine raylib mapmod nlohmann_json nfd)

# Compiler definitions
target_compile_definitions(ChainedDecosMapEditor PRIVATE
    PROJECT_ROOT_DIR="${CMAKE_SOURCE_DIR}"
)

# Use forward slashes for cross-platform compatibility
file(TO_CMAKE_PATH "${CMAKE_SOURCE_DIR}" PROJECT_ROOT_DIR_ESCAPED)
target_compile_definitions(ChainedDecosMapEditor PRIVATE
    PROJECT_ROOT_DIR="${PROJECT_ROOT_DIR_ESCAPED}"
)

# Set C++ standard
set_target_properties(ChainedDecosMapEditor PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)

# Create resources directory in build directory
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/resources/icons)

# Copy resources to build directory (check if file exists first)
if(EXISTS ${CMAKE_SOURCE_DIR}/resources/icons/ChainedDecosMapEditor.png)
    configure_file(${CMAKE_SOURCE_DIR}/resources/icons/ChainedDecosMapEditor.png ${CMAKE_BINARY_DIR}/resources/icons/ChainedDecosMapEditor.png COPYONLY)
else()
    message(WARNING "Icon file not found: ${CMAKE_SOURCE_DIR}/resources/icons/ChainedDecosMapEditor.png")
endif()
configure_file(${CMAKE_SOURCE_DIR}/imgui.ini ${CMAKE_BINARY_DIR}/imgui.ini COPYONLY)

# Installation
install(TARGETS ChainedDecosMapEditor
    RUNTIME DESTINATION bin
)

# Custom target for running MapEditor
add_custom_target(run_mapeditor
    COMMAND ChainedDecosMapEditor
    DEPENDS ChainedDecosMapEditor
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running ChainedDecos MapEditor"
)

# Documentation
set(DOXYGEN_INPUT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(DOXYGEN_OUTPUT_DIR ${CMAKE_BINARY_DIR}/doc/mapeditor)

# Optional: Enable testing
option(BUILD_MAPEDITOR_TESTS "Build MapEditor tests" OFF)
if(BUILD_MAPEDITOR_TESTS)
    add_subdirectory(tests)
endif()
