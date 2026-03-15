#ifndef CH_APPLICATION_H
#define CH_APPLICATION_H

#include "engine/core/assert.h"
#include "engine/core/base.h"
#include "engine/core/events.h"
#include "engine/core/imgui_layer.h"
#include "engine/core/layer_stack.h"
#include "engine/core/thread_pool.h"
#include "engine/core/timestep.h"
#include "engine/core/window.h"
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>


namespace CHEngine
{
class ImGuiLayer;
class Layer;

struct ApplicationCommandLineArgs
{
    int Count = 0;
    char** Args = nullptr;

    const char* operator[](int index) const
    {
        CH_CORE_ASSERT(index < Count);
        return Args[index];
    }
};

struct ApplicationSpecification
{
    std::string Name = "Chained Application";
    std::string WorkingDirectory;
    ApplicationCommandLineArgs CommandLineArgs;
    std::string ImGuiConfigurationPath = "imgui.ini";
    bool Headless = false;
};

// The main entry point and controller for the engine life cycle.
class Application
{
public:
    Application(const ApplicationSpecification& specification);
    virtual ~Application();

    void Close()
    {
        m_Running = false;
    }
    void Run();

    void PushLayer(Layer* layer);
    void PushOverlay(Layer* overlay);

    void OnEvent(Event& e);

    static Application& Get()
    {
        return *s_Instance;
    }

    Window& GetWindow();
    ImGuiLayer* GetImGuiLayer()
    {
        return m_ImGuiLayer;
    }
    const ApplicationSpecification& GetSpecification() const
    {
        return m_Specification;
    }
    LayerStack& GetLayerStack();

    void SubmitToMainThread(const std::function<void()>& function);

private:
    bool OnWindowClose(WindowCloseEvent& e);
    bool OnWindowResize(WindowResizeEvent& e);
    void ExecuteMainThreadQueue();

private:
    static Application* s_Instance;

    ApplicationSpecification m_Specification;
    std::unique_ptr<Window> m_Window;
    ImGuiLayer* m_ImGuiLayer = nullptr;

    bool m_Running = true;
    bool m_Minimized = false;

    Timestep m_DeltaTime = 0.0f;
    Timestep m_LastFrameTime = 0.0f;

    std::unique_ptr<LayerStack> m_LayerStack;
    
    // Subsystem pointers for safe cleanup
    class ThreadPool* m_ThreadPool = nullptr;
    class Renderer* m_Renderer = nullptr;
    class Audio* m_Audio = nullptr;
    class PhysicsSystem* m_PhysicsSystem = nullptr;
    class ComponentSerializer* m_ComponentSerializer = nullptr;

    std::vector<std::function<void()>> m_MainThreadQueue;
    std::mutex m_MainThreadQueueMutex;
};

Application* CreateApplication(ApplicationCommandLineArgs args);

} // namespace CHEngine

#endif // CH_APPLICATION_H
