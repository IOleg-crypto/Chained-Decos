cmake_minimum_required(VERSION 3.20)
project(ChainedDecosMapEditor)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(${PROJECT_NAME}
    Application.cpp
    Application.h
    Editor/Editor.h
    Editor/Editor.cpp
    Editor/CameraModes.h
    Editor/CameraModes.cpp
    Editor/MapObject.cpp
    Editor/MapObject.h
    Editor/ObjectTransform.h
    Editor/ObjectTransform.cpp
    Editor/UndoRedo.h
    Editor/UndoRedo.cpp
    Editor/ClipboardManager.h
    Editor/ClipboardManager.cpp
    Editor/LayerManager.h
    Editor/LayerManager.cpp
    Editor/ShortcutManager.h
    Editor/ShortcutManager.cpp
    Editor/ObjectFilter.h
    Editor/ObjectFilter.cpp
    Editor/StatusBar.h
    Editor/StatusBar.cpp
    Editor/EnhancedEditor.h
    MapFileManager/MapFileManager.h
    MapFileManager/MapFileManager.cpp
    MapFileManager/JsonMapFileManager.h
    MapFileManager/JsonMapFileManager.cpp
    main.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src/
    ${CMAKE_SOURCE_DIR}/src/Engine
    ${CMAKE_SOURCE_DIR}/src/Game
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    ChainedDecosEngine ChainedDecosGame
)
