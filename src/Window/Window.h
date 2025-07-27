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

/**
 *  @brief Window - class that creates window using Raylib library
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
public:
    Window() = default;
    Window(int screenX , int screenY , std::string windowName);
    ~Window();

    // Initialize the window
    void Init();

    // Run the main game loop
    void Run();
    // Init all keyboard hotkeys
    void KeyboardShortcut();
};

#endif // WINDOW_H

