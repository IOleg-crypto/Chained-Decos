#ifndef ENGINE_APPLICATION_H
#define ENGINE_APPLICATION_H

#include "core/Engine.h"
#include "core/application/IApplication.h"
#include "core/layer/LayerStack.h"
#include "core/utils/Base.h"
#include "events/Event.h"
#include <memory>
#include <string>


namespace CHEngine
{

// Engine Runtime - Runs the application
/**
 * @brief Main engine application wrapper (Host)
 */
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

public:
    EngineApplication(Config config, IApplication *application);
    ~EngineApplication();

    // --- Application Lifecycle ---
public:
    void Run();

    // --- Layer Management ---
public:
    void PushLayer(Layer *layer);
    void PushOverlay(Layer *overlay);
    void PopLayer(Layer *layer);
    void PopOverlay(Layer *overlay);

    // --- Events ---
public:
    void OnEvent(Event &e);

    // --- Getters & Config ---
public:
    CHEngine::Engine *GetEngine() const;
    Config &GetConfig();
    const Config &GetConfig() const;

    // --- Internal Helpers ---
private:
    void Initialize();
    void Shutdown();
    void Update();
    void Render();

    // --- Member Variables ---
private:
    IApplication *m_app;
    Config m_config;
    std::shared_ptr<CHEngine::Engine> m_engine;
    LayerStack m_LayerStack;
    bool m_initialized = false;
};

} // namespace CHEngine

#endif // ENGINE_APPLICATION_H
