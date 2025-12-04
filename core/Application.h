#ifndef CORE_APPLICATION_H
#define CORE_APPLICATION_H

namespace Core
{

// Forward declaration
struct EngineConfig;
class Engine;

// Base application class - derive to create your game
class Application
{
public:
    virtual ~Application() = default;

    // Called before engine initialization to configure settings
    virtual void OnConfigure(EngineConfig &config) = 0;

    // Called after engine initialization
    virtual void OnStart() = 0;

    // Called every frame
    virtual void OnUpdate(float delta_time) = 0;

    // Called every frame for rendering
    virtual void OnRender() = 0;

    // Called before engine shutdown
    virtual void OnShutdown() = 0;

    // Set engine reference (called by Engine)
    void SetEngine(Engine *engine)
    {
        m_engine = engine;
    }

protected:
    Engine *m_engine = nullptr;
};

} // namespace Core

#endif // CORE_APPLICATION_H
