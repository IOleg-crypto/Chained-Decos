//
// Created by I#Oleg on 20.07.2025.
//

#ifndef WINDOW_H
#define WINDOW_H

#include <string>
#include <memory>
#include <raylib.h>
#include "../Player/Player.h"
#include "../Model/Model.h"

#include "rcamera.h"

/**
 *  Window - class that creates window using Raylib library
 */
class Window {
private:
    // Screen resolution
    int m_screenX;
    int m_screenY;

    // Window title
    std::string m_WindowName;

    // Game objects
    Player m_player;
    Models m_models;
private:
    bool m_showDebug;
public:
    Window() = default;
    Window(int screenX , int screenY , std::string windowName);
    ~Window();

    // Deleted constructors(useless)
public:
     Window(const Window &other) = delete;
     Window(Window &&other) = delete;
public:

    // Initialize the window
    void Init();

    // Run the main game loop
    void Run();
    // Init all keyboard hotkeys
    void KeyboardShortcut();
    // Render all stuff , models , text , etc
    void Render();
    // Draw models in 3D
    void DrawScene3D();
    // Useful for keyboard
    void Update();
    // Show debug info
    static void DrawDebugInfo(const Camera &camera , const int &cameraMode);
};

#endif // WINDOW_H

