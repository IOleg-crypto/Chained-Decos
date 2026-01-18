#ifndef CH_APPLICATION_H
#define CH_APPLICATION_H

#include "engine/core/base.h"
#include "engine/core/events.h"
#include "engine/core/layer.h"
#include "engine/core/layer_stack.h"
#include "engine/core/window.h"
#include "engine/scene/scene.h"
#include <string>

namespace CHEngine
{

struct ApplicationConfig
{
    std::string Title = "Chained Engine";
    int Width = 1280;
    int Height = 720;
    bool Fullscreen = false;
    int TargetFPS = 60;
};

class Application
{
public:
    using Config = ApplicationConfig;

public:
    Application(const Config &config = Config());
    virtual ~Application();

    static void Init(const Config &config);
    static void Shutdown();

    static void PushLayer(Layer *layer);
    static void PushOverlay(Layer *overlay);

    static bool ShouldClose();
    static void BeginFrame();
    static void EndFrame();
    static void PollEvents();
    static void OnEvent(Event &e);

    void Run();

    LayerStack &GetLayerStack()
    {
        return m_LayerStack;
    }

    static bool IsRunning();
    static float GetDeltaTime();

    static Application &Get()
    {
        return *s_Instance;
    }

    Scope<Window> &GetWindow()
    {
        return m_Window;
    }

    Ref<Scene> GetActiveScene()
    {
        return m_ActiveScene;
    }
    void SetActiveScene(Ref<Scene> scene)
    {
        m_ActiveScene = scene;
    }
    void LoadScene(const std::string &path);

private:
    static Application *s_Instance;

    bool m_Running = false;
    float m_DeltaTime = 0.0f;
    LayerStack m_LayerStack;
    Scope<Window> m_Window;
    Ref<Scene> m_ActiveScene;
};

// To be defined by CLIENT
Application *CreateApplication();
} // namespace CHEngine

#endif // CH_APPLICATION_H
