#ifndef ENGINE_APPLICATION_H
#define ENGINE_APPLICATION_H

#include "Types.h"

namespace Core
{

class Engine; // Forward decl only for pointer

// Base Application class
class Application
{
public:
    virtual ~Application() = default;

    // Lifecycle
    virtual void OnConfigure(EngineConfig &config)
    {
    }
    virtual void OnStart()
    {
    }
    virtual void OnUpdate(float deltaTime)
    {
    }
    virtual void OnRender()
    {
    }
    virtual void OnShutdown()
    {
    }

    // Access to engine
    Engine *GetEngine() const
    {
        return m_engine;
    }

protected:
    Engine *m_engine = nullptr;
    friend class Engine;
};

} // namespace Core

#endif // ENGINE_APPLICATION_H
