//
// EngineApplication.h - Base application class for all projects using the engine
// Created by Auto on 2025
//
#ifndef ENGINE_APPLICATION_H
#define ENGINE_APPLICATION_H

#include "Engine/Engine.h"
#include "Engine/Kernel/Kernel.h"
#include "Engine/Module/ModuleManager.h"
#include <memory>
#include <string>
#include <functional>

// Base class for all projects using the engine
// Provides full lifecycle and access to all engine resources
class EngineApplication {
public:
    struct Config {
        int width;
        int height;
        std::string windowName;
        bool enableMSAA;
        bool resizable;
        
        Config() : width(1280), height(720), windowName("Engine Application"), 
                   enableMSAA(true), resizable(true) {}
    };

    explicit EngineApplication(const Config& config = Config());
    virtual ~EngineApplication();

    // Main lifecycle loop (called from main)
    void Run();
    
    // Public API for engine access
    Engine* GetEngine() const { return m_engine.get(); }
    Kernel* GetKernel() const { return m_kernel.get(); }
    ModuleManager* GetModuleManager() const { 
        return m_engine ? m_engine->GetModuleManager() : nullptr; 
    }
    
    // Configuration (called before Init)
    void SetConfig(const Config& config) { m_config = config; }
    Config& GetConfig() { return m_config; }
    const Config& GetConfig() const { return m_config; }
    
protected:
    // ===== VIRTUAL METHODS FOR EXTENSION =====
    // Projects can override for customization
    
    // Called before Engine initialization
    virtual void OnPreInitialize() {}
    
    // Called after creating Engine, Kernel, but before Init()
    virtual void OnInitializeServices() {}
    
    // Called after registering Engine services
    virtual void OnRegisterEngineServices() {}
    
    // Called to register project modules (must override)
    virtual void OnRegisterProjectModules() = 0;
    
    // Called to register project services
    virtual void OnRegisterProjectServices() {}
    
    // Called before initializing all modules
    virtual void OnPreInitializeModules() {}
    
    // Called after full initialization
    virtual void OnPostInitialize() {}
    
    // Called before each frame update
    virtual void OnPreUpdate(float deltaTime) {}
    
    // Called after updating Engine and modules
    virtual void OnPostUpdate(float deltaTime) {}
    
    // Called before rendering
    virtual void OnPreRender() {}
    
    // Called after rendering Engine and modules
    virtual void OnPostRender() {}
    
    // Called before shutdown
    virtual void OnPreShutdown() {}
    
    // Called for command line processing (optional)
    virtual void ProcessCommandLine(int argc, char* argv[]) {}
    
    // Configure kernel before initialization
    virtual void ConfigureKernel(Kernel* kernel) {}
    
    // Configure module manager
    virtual void ConfigureModuleManager(ModuleManager* manager) {}

private:
    void Initialize();
    void Shutdown();
    void Update();
    void Render();
    
    Config m_config;
    std::unique_ptr<Kernel> m_kernel;
    std::unique_ptr<Engine> m_engine;
    bool m_initialized = false;
};

#endif // ENGINE_APPLICATION_H

