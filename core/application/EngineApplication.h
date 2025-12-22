#ifndef ENGINE_APPLICATION_H
#define ENGINE_APPLICATION_H

#include "core/Engine.h"
#include "core/application/IApplication.h"
#include "events/Event.h"
#include "core/layer/LayerStack.h"
#include "core/utils/Base.h"
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
        bool fullscreen = false;
        bool vsync = true;
    };

    EngineApplication(Config config, IApplication *application);
    ~EngineApplication();

    void Run();

    void PushLayer(Layer *layer);
    void PushOverlay(Layer *overlay);
    void PopLayer(Layer *layer);
    void PopOverlay(Layer *overlay);

    void OnEvent(Event &e);

    ChainedEngine::Engine *GetEngine() const;

    // Configuration
    Config &GetConfig();
    const Config &GetConfig() const;

private:
    void Initialize();
    void Shutdown();
    void Update();
    void Render();

    IApplication *m_app; // The application instance
    Config m_config;
    std::shared_ptr<ChainedEngine::Engine> m_engine; // Engine is now shared/singleton managed
    LayerStack m_LayerStack;
    bool m_initialized = false;
};

} // namespace ChainedDecos

#endif // ENGINE_APPLICATION_H
