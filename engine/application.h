#ifndef CH_APPLICATION_H
#define CH_APPLICATION_H

#include "engine/events.h"
#include "engine/layer_stack.h"
#include "engine/types.h"
#include <raylib.h>
#include <string>

namespace CH
{
class Layer;
class Event;

struct ApplicationConfig
{
    std::string Title = "Chained Engine";
    int Width = 1280;
    int Height = 720;
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
    static void OnEvent(Event &e);

    void Run();

    static bool IsRunning();
    static float GetDeltaTime();

    static Application &Get()
    {
        return *s_Instance;
    }

private:
    static Application *s_Instance;

    bool m_Running = false;
    float m_DeltaTime = 0.0f;
    LayerStack m_LayerStack;
};

// To be defined by CLIENT
Application *CreateApplication();
} // namespace CH

#endif // CH_APPLICATION_H
