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

#include <memory>
#include <string>
// Project headers
#include "core/object/kernel/Core/Kernel.h"
#include "core/object/kernel/Interfaces/IEngine.h"
#include "core/object/kernel/Interfaces/IKernelService.h"
#include "core/object/module/Core/ModuleManager.h"
#include "servers/input/Core/InputManager.h"
#include "servers/rendering/Core/RenderManager.h"

// Configuration for Engine initialization
struct EngineConfig
{
    int screenWidth = 1920;
    int screenHeight = 1080;
    std::shared_ptr<RenderManager> renderManager;
    std::shared_ptr<InputManager> inputManager;
    Kernel *kernel = nullptr;
};

CHAINEDDECOSENGINE_API class Engine : public IEngine
{
public:
    explicit Engine(const EngineConfig &config);
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
    // Getters
    RenderManager *GetRenderManager() const override
    {
        return m_renderManager.get();
    }
    InputManager &GetInputManager() const override
    {
        return *m_inputManager;
    }
    Kernel *GetKernel() const
    {
        return m_kernel.get();
    }
    ModuleManager *GetModuleManager() override
    {
        return m_moduleManager.get();
    }

    // Configuration
    const EngineConfig &GetConfig() const
    {
        return m_config;
    }

    void RegisterModule(std::unique_ptr<class IEngineModule> module);

    // ==================== Engine State Control ====================
    void RequestExit();
    void SetWindowName(const std::string &name);

    bool IsDebugInfoVisible() const override;
    bool IsCollisionDebugVisible() const override;

private:
    void HandleEngineInput(); // For engine-level shortcuts (e.g., F11 for fullscreen)

private:
    EngineConfig m_config;

    // Window & Display
    int m_screenX;
    int m_screenY;
    std::string m_windowName;
    bool m_windowInitialized; // Track if this Engine instance initialized the window

    // Core systems
    std::unique_ptr<Kernel> m_kernel;
    std::unique_ptr<ModuleManager> m_moduleManager;
    std::shared_ptr<RenderManager> m_renderManager;
    std::shared_ptr<InputManager> m_inputManager;

    // Engine State
    bool m_shouldExit;
    bool m_isEngineInit;
    bool m_initialized;
    bool m_running;
};

#include "Kernel/Interfaces/IKernelService.h"

struct EngineService : public IKernelService
{
    Engine *engine = nullptr;
    explicit EngineService(Engine *e) : engine(e)
    {
    }
    bool Initialize() override
    {
        return engine != nullptr;
    }
    void Shutdown() override
    {
    }
    void Update(float deltaTime) override
    {
    }
    void Render() override
    {
    }
    const char *GetName() const override
    {
        return "EngineService";
    }
};

#endif // ENGINE_H