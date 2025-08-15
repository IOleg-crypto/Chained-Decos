//
// Created by I#Oleg
//
#ifndef APPLICATION_H
#define APPLICATION_H

#include <imgui.h>
#include <iostream>
#include <memory>
#include <raylib.h>
#include <rlImGui.h>
#include <string>

#include "Editor/Editor.h"

// Main application class for the map editor
class Application
{
public:
    // Constructor and destructor
    Application(int width, int height); // Initialize with window dimensions
    ~Application();
    // I don`t need this because i use unique pointer
    Application(const Application &other) = delete;
    Application(Application &&other) = delete;

public:
    // Core application functions
    void Init() const; // Initialize the application and window
    void Run() const;  // Main application loop

private:
    int m_width;                      // Window width
    int m_height;                     // Window height
    std::string m_windowName;         // Window title
    std::unique_ptr<Editor> m_editor; // Main editor instance
    Image m_icon;                     // Window icon image
};

#endif // APPLICATION_H