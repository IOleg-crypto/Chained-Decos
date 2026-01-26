#ifndef CH_APPLICATION_H
#define CH_APPLICATION_H

#include "engine/core/base.h"
#include "engine/core/events.h"
#include "engine/core/layer.h"
#include "engine/core/layer_stack.h"
#include "engine/core/window.h"
#include "engine/scene/scene.h"
#include <atomic>
#include <mutex>
#include <string>
#include <thread>

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

    bool Initialize(const Config &config);
    void Shutdown();
    void Close();

    static void PushLayer(Layer *layer);
    static void PushOverlay(Layer *overlay);

    static bool ShouldClose();
    static void BeginFrame();
    static void EndFrame();
    static void PollEvents();
    static void OnEvent(Event &e);

    void Run();

    virtual void PostInitialize()
    {
    }

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

    std::unique_ptr<Window> &GetWindow()
    {
        return m_Window;
    }

    std::shared_ptr<Scene> GetActiveScene()
    {
        return m_ActiveScene;
    }
    void SetActiveScene(std::shared_ptr<Scene> scene)
    {
        m_ActiveScene = scene;
    }

    void RequestSceneChange(const std::string &path)
    {
        m_NextScenePath = path;
    }

    void LoadScene(const std::string &path);

    const Config &GetConfig() const
    {
        return m_Config;
    }

private:
    void ProcessEvents();
    void Simulate();
    void Animate();
    void Render();

    void OnUpdate(float deltaTime);
    void OnRender();

    static Application *s_Instance;

    Config m_Config;
    bool m_Running = false;
    bool m_Minimized = false;
    float m_DeltaTime = 0.0f;
    float m_LastFrameTime = 0.0f;

    LayerStack m_LayerStack;
    std::unique_ptr<Window> m_Window;
    std::shared_ptr<Scene> m_ActiveScene;
    std::string m_NextScenePath;
};

// To be defined by CLIENT
Application *CreateApplication();
} // namespace CHEngine

#endif // CH_APPLICATION_H
