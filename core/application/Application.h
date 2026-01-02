#ifndef CD_CORE_APPLICATION_APPLICATION_H
#define CD_CORE_APPLICATION_APPLICATION_H

#include "core/layer/layer_stack.h"
#include "core/utils/base.h"
#include "core/window/window.h"
#include "events/application_event.h"
#include "events/event.h"


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

private:
    static Application *s_Instance;
};

// To be defined in CLIENT
Application *CreateApplication(int argc, char **argv);

} // namespace CHEngine

#endif // CD_CORE_APPLICATION_APPLICATION_H
