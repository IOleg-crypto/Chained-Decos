#ifndef ENGINE_APPLICATION_H
#define ENGINE_APPLICATION_H

#include "Base.h"
#include "Engine.h"
#include "IApplication.h"
#include "LayerStack.h"
#include "core/events/Event.h"
#include <memory>
#include <string>

namespace ChainedDecos
{

// Engine Runtime - Runs the application
class EngineApplication
{
public:
    struct Config
    {
        int width = 1280;
        int height = 720;
        std::string windowName = "Engine Application";
        bool enableMSAA = true;
        bool resizable = true;
    };

    EngineApplication(Config config, IApplication *application);
    ~EngineApplication();

    void Run();

    void PushLayer(Layer *layer);
    void PushOverlay(Layer *overlay);

    void OnEvent(Event &e);

    Engine *GetEngine() const
    {
        return m_engine.get();
    }

    // Configuration
    Config &GetConfig()
    {
        return m_config;
    }
    const Config &GetConfig() const
    {
        return m_config;
    }

private:
    void Initialize();
    void Shutdown();
    void Update();
    void Render();

    IApplication *m_app; // The application instance
    Config m_config;
    std::shared_ptr<Engine> m_engine; // Engine is now shared/singleton managed
    LayerStack m_LayerStack;
    bool m_initialized = false;
};

} // namespace ChainedDecos

#endif // ENGINE_APPLICATION_H
