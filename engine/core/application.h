#ifndef CH_APPLICATION_H
#define CH_APPLICATION_H

#include "engine/core/base.h"
#include "engine/core/events.h"
#include "engine/core/layer.h"
#include "engine/core/layer_stack.h"
#include "engine/core/window.h"
#include "engine/renderer/render_state.h"
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
    virtual void OnUpdate(float deltaTime);
    virtual void OnRender();

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

    const Config &GetConfig() const
    {
        return m_Config;
    }

    // Threading access
    std::mutex &GetSimMutex()
    {
        return m_SimMutex;
    }

private:
    void UpdateSimulation();
    static Application *s_Instance;

    Config m_Config;
    bool m_Running = false;
    bool m_Minimized = false;
    float m_DeltaTime = 0.0f;
    float m_LastFrameTime = 0.0f;
    float m_FixedTimeAccumulator = 0.0f;
    const float m_FixedStep = 1.0f / 60.0f;

    LayerStack m_LayerStack;
    Scope<Window> m_Window;
    Ref<Scene> m_ActiveScene;

    // Simulation Threading
    std::thread m_SimulationThread;
    std::mutex m_SimMutex;
    std::atomic<bool> m_IsSimulating{false};

    // Render Snapshotting
    RenderState m_RenderStates[3];
    uint32_t m_SimBufferIndex = 0;
    uint32_t m_RenderBufferIndex = 1;
    uint32_t m_PendingBufferIndex = 2;
    std::atomic<bool> m_NewStateAvailable{false};
};

// To be defined by CLIENT
Application *CreateApplication();
} // namespace CHEngine

#endif // CH_APPLICATION_H
