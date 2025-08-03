//
// Created by I#Oleg
//
#ifndef APPLICATION_H
#define APPLICATION_H

#include <raylib.h>
#include <string>
#include <memory>
#include <imgui.h>
#include <rlImGui.h>
#include <iostream>

#include "Editor/Editor.h"

// Main application class for the map editor
class Application {
public:
    // Constructor and destructor
    Application(int width, int height);  // Initialize with window dimensions
    ~Application();
    Application(const Application&other) = delete;  // Disable copy constructor
    Application(Application &&other) = delete;      // Disable move constructor
    
public:
    // Core application functions
    void Init() const;    // Initialize the application and window
    void Run() const;     // Main application loop
    
private:
    int m_width;                    // Window width
    int m_height;                   // Window height
    std::string m_windowName;       // Window title
    std::unique_ptr<Editor> m_editor;  // Main editor instance
};

#endif // APPLICATION_H 