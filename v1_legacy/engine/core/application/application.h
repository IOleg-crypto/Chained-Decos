#ifndef CD_ENGINE_CORE_APPLICATION_APPLICATION_H
#define CD_ENGINE_CORE_APPLICATION_APPLICATION_H

#include "core/utils/base.h"
#include "engine/core/layer/layer_stack.h"
#include "engine/core/window/window.h"
#include "events/application_event.h"
#include "events/event.h"
#include <memory>
#include <string>
#include <vector>


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

    Window &GetWindow()
    {
        return *m_Window;
    }
    static Application &Get()
    {
        return *s_Instance;
    }

private:
    bool OnWindowClose(WindowCloseEvent &e);
    bool OnWindowResize(WindowResizeEvent &e);

    std::unique_ptr<Window> m_Window;
    bool m_Running = true;
    bool m_Minimized = false;
    LayerStack m_LayerStack;
    std::vector<Layer *> m_LayerDeletionQueue;
    float m_LastFrameTime = 0.0f;

    static Application *s_Instance;
};

Application *CreateApplication(int argc, char **argv);
} // namespace CHEngine

#endif
