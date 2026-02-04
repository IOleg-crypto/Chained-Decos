#ifndef CH_APPLICATION_H
#define CH_APPLICATION_H

#include "engine/core/base.h"
#include "engine/core/assert.h"
#include "engine/core/events.h"
#include "engine/core/layer_stack.h"
#include "engine/core/timestep.h"
#include "engine/core/window.h"
#include "engine/core/thread_pool.h"
#include "engine/core/imgui_layer.h"

#include <string>
#include <memory>
#include <mutex>
#include <functional>
#include <vector>

namespace CHEngine
{

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

        bool EnableDocking = true;
        bool EnableViewports = true;
        std::string IniFilename = "imgui.ini";
    };

    // The main entry point and controller for the engine life cycle.
    class Application
    {
    public:
        Application(const ApplicationSpecification &specification);
        virtual ~Application();

        void Close() { m_Running = false; }
        void Run();

        void PushLayer(Layer *layer);
        void PushOverlay(Layer *overlay);

        void OnEvent(Event &e);

        static Application& Get() { return *s_Instance; }

        Window& GetWindow() { return *m_Window; }
        ImGuiLayer* GetImGuiLayer() { return m_ImGuiLayer; }
        const ApplicationSpecification& GetSpecification() const { return m_Specification; }
        LayerStack& GetLayerStack() { return m_LayerStack; }
        ThreadPool& GetThreadPool() { return *m_ThreadPool; }

        void SubmitToMainThread(const std::function<void()>& function);

    private:
        bool OnWindowClose(WindowCloseEvent& e);
        bool OnWindowResize(WindowResizeEvent& e);
        void ExecuteMainThreadQueue();

    private:
        static Application *s_Instance;

        ApplicationSpecification m_Specification;
        std::unique_ptr<Window> m_Window;
        std::unique_ptr<ThreadPool> m_ThreadPool;
        ImGuiLayer* m_ImGuiLayer = nullptr;
        
        bool m_Running = true;
        bool m_Minimized = false;
        
        Timestep m_DeltaTime = 0.0f;
        float m_LastFrameTime = 0.0f;

        LayerStack m_LayerStack;

        std::vector<std::function<void()>> m_MainThreadQueue;
        std::mutex m_MainThreadQueueMutex;
    };

    Application *CreateApplication(ApplicationCommandLineArgs args);

} // namespace CHEngine

#endif // CH_APPLICATION_H
