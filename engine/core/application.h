#ifndef CH_APPLICATION_H
#define CH_APPLICATION_H

#include "engine/core/base.h"
#include "engine/core/engine_service.h"
#include "engine/core/events.h"
#include "engine/core/layer_stack.h"
#include "engine/core/service_locator.h"
#include "engine/core/timestep.h"
#include "engine/core/window.h"

#include <memory>
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
};

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
    bool Headless = false;
    bool EnableScripting = true;
    std::string ImGuiConfigurationPath = "imgui.ini";
};

class CH_API Application
{
public:
    Application(const ApplicationSpecification& specification);
    virtual ~Application();

    void Run();
    void Close()
    {
        m_Running = false;
    }

    void PushLayer(std::unique_ptr<Layer> layer);
    void PushOverlay(std::unique_ptr<Layer> overlay);

    void OnEvent(Event& e);

    static Application& Get()
    {
        return *s_Instance;
    }

    Window& GetWindow()
    {
        return *m_Window;
    }
    ImGuiLayer* GetImGuiLayer()
    {
        return m_ImGuiLayer;
    }
    const ApplicationSpecification& GetSpecification() const
    {
        return m_Specification;
    }
    LayerStack& GetLayerStack()
    {
        return *m_LayerStack;
    }
    static std::filesystem::path GetExecutableDirectory();
    Timestep GetFrameTime() const
    {
        return m_Timer.DeltaTime;
    }

    template <typename T, typename... Args> T& AddService(Args&&... args)
    {
        auto service = std::make_unique<T>(std::forward<Args>(args)...);
        T& ref = *service;
        ServiceLocator::Register<T>(&ref);
        m_Services.push_back(std::move(service));
        return ref;
    }

private:
    void ReplicateEntities();

private:
    static Application* s_Instance;

    ApplicationSpecification m_Specification;
    std::unique_ptr<Window> m_Window;
    std::unique_ptr<LayerStack> m_LayerStack;
    ImGuiLayer* m_ImGuiLayer = nullptr;

    std::vector<std::unique_ptr<EngineService>> m_Services;

    struct
    {
        float Accumulator = 0.0f;
        float FixedStepCount = 1.0f / 120.0f;
        Timestep DeltaTime = 0.0f;
        float LastFrameTime = 0.0f;
    } m_Timer;

    bool m_Running = false;
};

Application* CreateApplication(ApplicationCommandLineArgs args);
} // namespace CHEngine

#endif // CH_APPLICATION_H
