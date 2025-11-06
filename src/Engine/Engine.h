//
// Engine.h - Main Engine Class
// Created by I#Oleg on 20.07.2025.
//
#ifndef ENGINE_H
#define ENGINE_H

#ifdef _WIN32
#ifdef CHAINEDDECOSENGINE_EXPORTS
#define CHAINEDDECOSENGINE_API __declspec(dllexport)
#else
#define CHAINEDDECOSENGINE_API __declspec(dllimport)
#endif
#else
#define CHAINEDDECOSENGINE_API
#endif

#include <string>
#include <memory>
// Project headers
#include "Input/InputManager.h"
#include "Kernel/Kernel.h"
#include "Render/RenderManager.h"

// Forward declaration
class ModuleManager;

CHAINEDDECOSENGINE_API class Engine
{
public:
    Engine(std::shared_ptr<RenderManager> renderManager, std::shared_ptr<InputManager> inputManager, Kernel* kernel = nullptr);
    Engine(int screenX, int screenY, std::shared_ptr<RenderManager> renderManager, std::shared_ptr<InputManager> inputManager, Kernel* kernel = nullptr);
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
    [[nodiscard]] RenderManager *GetRenderManager() const;

    InputManager &GetInputManager() const;
    
    Kernel* GetKernel() const { return m_kernel; }

    // ==================== Module System ====================
    ModuleManager* GetModuleManager() { return m_moduleManager.get(); }
    const ModuleManager* GetModuleManager() const { return m_moduleManager.get(); }

    void RegisterModule(std::unique_ptr<class IEngineModule> module);

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
    std::shared_ptr<InputManager> m_inputManager;
    std::shared_ptr<RenderManager> m_renderManager;
    Kernel* m_kernel;
    
    // Module System
    std::unique_ptr<ModuleManager> m_moduleManager;

    // Engine State
    bool m_shouldExit;
    bool m_isEngineInit;
};

#endif // ENGINE_H