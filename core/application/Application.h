#ifndef APPLICATION_H
#define APPLICATION_H

#include "core/Base.h"
#include "core/layer/LayerStack.h"
#include "core/window/Window.h"
#include "events/ApplicationEvent.h"
#include "events/Event.h"

#include <memory>

namespace CHEngine
{
class Application
{
public:
    Application(const std::string &name = "Chained Engine App");
    virtual ~Application();

    void Run();

    virtual void OnEvent(Event &e);

    void PushLayer(Layer *layer);
    void PushOverlay(Layer *overlay);
    void PopLayer(Layer *layer);
    void PopOverlay(Layer *overlay);

    void Close()
    {
        m_Running = false;
    }

    bool IsCollisionDebugVisible() const
    {
        return m_CollisionDebug;
    }
    void SetCollisionDebugVisible(bool visible)
    {
        m_CollisionDebug = visible;
    }

    bool IsDebugInfoVisible() const
    {
        return m_DebugInfo;
    }
    void SetDebugInfoVisible(bool visible)
    {
        m_DebugInfo = visible;
    }

    inline Window &GetWindow()
    {
        return *m_Window;
    }

    inline static Application &Get()
    {
        return *s_Instance;
    }

private:
    bool OnWindowClose(WindowCloseEvent &e);
    bool OnWindowResize(WindowResizeEvent &e);

private:
    std::unique_ptr<Window> m_Window;
    bool m_Running = true;
    bool m_Minimized = false;
    LayerStack m_LayerStack;
    std::vector<Layer *> m_LayerDeletionQueue;
    float m_LastFrameTime = 0.0f;

    bool m_CollisionDebug = false;
    bool m_DebugInfo = false;

private:
    static Application *s_Instance;
};

// To be defined in CLIENT
Application *CreateApplication(int argc, char **argv);

} // namespace CHEngine

#endif // APPLICATION_H
