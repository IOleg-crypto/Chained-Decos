//
// Engine.h - Main Engine Class
// Created by I#Oleg on 20.07.2025.
//
#ifndef ENGINE_H
#define ENGINE_H

#include <string>
// Project headers
#include "Input/InputManager.h"
#include "Render/RenderManager.h"


class RenderManager; // To bad!!!
/**
 * Main Engine class - manages the core application window and rendering
 *
 * Responsibilities:
 *  - Window creation and management (using raylib)
 *  - Basic game loop (update/render cycle for engine itself)
 *  - Managing core engine services (input, rendering)
 *  - Debug information handling
 *
 */
class Engine
{
public:
    Engine();
    Engine(int screenX, int screenY);
    ~Engine();
    Engine(Engine &&other) = delete;
    Engine &operator=(const Engine &other) = delete;
    Engine &operator=(Engine &&other) = delete;

    // ==================== MAIN API ====================
    void Init();
    void Update();
    void Render() const;
    [[nodiscard]] bool ShouldClose() const;
    void Shutdown() const;

    // ==================== Public Getters for Engine Services ====================
    RenderManager *GetRenderManager();

    InputManager &GetInputManager();

    // ==================== Engine State Control ====================
    void RequestExit();
    bool IsDebugInfoVisible() const;

    bool IsCollisionDebugVisible() const;

private:
    void HandleEngineInput(); // For engine-level shortcuts (e.g., F11 for fullscreen)

private:
    // Window & Display
    int m_screenX;
    int m_screenY;
    std::string m_windowName;
    bool m_windowInitialized; // Track if this Engine instance initialized the window

    // Core Engine Services
    InputManager m_inputManager;
    RenderManager *m_renderManager;

    // Engine State
    bool m_shouldExit;

    // Debug State
    bool m_showDebug;
    bool m_showCollisionDebug;
    bool m_isEngineInit;
};

#endif // ENGINE_H