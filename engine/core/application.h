#ifndef CH_APPLICATION_H
#define CH_APPLICATION_H

#include "engine/core/ch_assert.h"
#include "engine/core/events.h"
#include "engine/core/layer_stack.h"
#include "engine/core/timestep.h"
#include "engine/core/window.h"
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include <filesystem>


namespace CHEngine
{
class ImGuiLayer;
class Layer;
class ScriptEngine;

// Command-line arguments passed into application startup.
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

// Construction parameters and persistent app settings.
struct ApplicationSpecification
{
    std::string Name = "Chained Application";
    std::string WorkingDirectory;
    int WindowWidth = 1280;
    int WindowHeight = 720;
    bool VSync = true;
    bool Fullscreen = false;
    bool Resizable = true;
    std::string AppIcon = "";
    
    ApplicationCommandLineArgs CommandLineArgs;
    std::string ImGuiConfigurationPath = "imgui.ini";
    bool Headless = false;
    bool EnableScripting = true;
    std::function<void()> InitScripting;
    std::function<void()> ShutdownScripting;
};

// Owns the window, layer stack, and main loop for the process.
class Application
{
public:
    Application(const ApplicationSpecification& specification);
public:
    virtual ~Application();

    // Requests the main loop to exit.
    void Close()
    {
        m_Running = false;
    }
    // Runs the frame loop until Close() is called.
    void Run();

    void PushLayer(std::unique_ptr<Layer> layer);
    void PushOverlay(std::unique_ptr<Layer> overlay);

    void OnEvent(Event& e);

    // Immediate dispatch (Hazel style)
    void DispatchEvent(Event& e);
    // Queued dispatch for safe processing at frame end
    template<typename T, typename... Args>
    void PostEvent(Args&&... args)
    {
        m_EventQueue.Enqueue<T>(std::forward<Args>(args)...);
    }

    static Application& Get()
    {
        return *s_Instance;
    }

    // Returns the directory containing the executable.
    static std::filesystem::path GetExecutableDirectory();

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
    float GetFrameTime() const { return m_DeltaTime; }



    // Schedules work to run on the main thread at a safe point in the frame.
    void SubmitToMainThread(const std::function<void()>& function);

private:
    bool OnWindowClose(WindowCloseEvent& e);
    bool OnWindowResize(WindowResizeEvent& e);
    void ExecuteMainThreadQueue();

private:
    static Application* s_Instance;

    ApplicationSpecification m_Specification;
    std::unique_ptr<LayerStack> m_LayerStack;
    EventQueue m_EventQueue;

    ImGuiLayer* m_ImGuiLayer = nullptr;
    std::unique_ptr<Window> m_Window;

    bool m_Running = false;
    bool m_Minimized = false;

    Timestep m_DeltaTime = 0.0f;
    Timestep m_LastFrameTime = 0.0f;

    float m_FixedTimestep = 1.0f / 60.0f;
    float m_Accumulator = 0.0f;

    std::vector<std::function<void()>> m_MainThreadQueue;
    std::mutex m_MainThreadQueueMutex;
};

Application* CreateApplication(ApplicationCommandLineArgs args);

} // namespace CHEngine

#endif // CH_APPLICATION_H
