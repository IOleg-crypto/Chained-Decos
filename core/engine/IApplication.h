#ifndef IAPPLICATION_H
#define IAPPLICATION_H

#include "Engine.h"
#include <string>

// Application interface - all methods receive Engine reference
class IApplication
{
public:
    struct EngineConfig
    {
        int width = 1280;
        int height = 720;
        std::string windowName = "Application";
    };

    virtual ~IApplication() = default;

    // Configuration (called before Engine creation)
    virtual void OnConfigure(EngineConfig &config) = 0;

    // Registration (called after Engine creation, before initialization)
    virtual void OnRegister(Engine &engine) = 0;

    // Lifecycle (all receive Engine reference)
    virtual void OnStart(Engine &engine) = 0;
    virtual void OnUpdate(float deltaTime, Engine &engine) = 0;
    virtual void OnRender(Engine &engine) = 0;
    virtual void OnShutdown() = 0;
};

#endif // IAPPLICATION_H
