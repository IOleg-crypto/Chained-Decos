//
// Created by I#Oleg
//
#ifndef APPLICATION_H
#define APPLICATION_H

#include "imgui.h"
#include "raylib.h"
#include "rlImGui.h"
#include <iostream>
#include <memory>
#include <string>

#include "Editor/Editor.h"

// Main application class for the map editor
class Application
{
public:
    // Constructor and destructor
    Application(int width, int height, std::unique_ptr<Editor> editor); // Initialize with window dimensions and editor
    ~Application();

public:
    // Core application functions
    void Init() const; // Initialize the application and window
    void Run() const;  // Main application loop

private:
    int m_width;                      // Window width
    int m_height;                     // Window height
    std::string m_windowName;         // Window title
    std::unique_ptr<Editor> m_editor; // Main editor instance
};

#endif // APPLICATION_H