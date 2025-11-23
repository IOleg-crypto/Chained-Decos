#pragma once

#include "core/object/kernel/Core/Kernel.h"
#include "core/object/module/Core/ModuleManager.h"
#include "servers/input/Core/InputManager.h"
#include "servers/rendering/Core/RenderManager.h"
#include <memory>


// Main Engine class
class Engine
{
public:
    Engine(Kernel *kernel);
    ~Engine();

    bool Initialize();
    void Shutdown();

    // Accessors
    Kernel *GetKernel() const
    {
        return m_kernel;
    }
    ModuleManager *GetModuleManager() const
    {
        return m_moduleManager.get();
    }
    RenderManager *GetRenderManager() const
    {
        return m_renderManager.get();
    }
    InputManager *GetInputManager() const
    {
        return m_inputManager.get();
    }

    // Module registration
    void RegisterModule(std::unique_ptr<IModule> module);

    // Debug
    bool IsDebugInfoVisible() const
    {
        return m_debugInfoVisible;
    }
    void SetDebugInfoVisible(bool visible)
    {
        m_debugInfoVisible = visible;
    }

private:
    Kernel *m_kernel;
    std::unique_ptr<ModuleManager> m_moduleManager;
    std::unique_ptr<RenderManager> m_renderManager;
    std::unique_ptr<InputManager> m_inputManager;
    bool m_debugInfoVisible = false;
};
