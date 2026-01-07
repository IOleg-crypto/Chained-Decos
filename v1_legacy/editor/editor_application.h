#ifndef CD_EDITOR_EDITOR_APPLICATION_H
#define CD_EDITOR_EDITOR_APPLICATION_H

#include "engine/core/application/application.h"

namespace CHEngine
{
class EditorLayer;
}

// Editor application - uses full engine + own layers
class EditorApplication : public CHEngine::Application
{
public:
    EditorApplication(int argc, char *argv[]);
    ~EditorApplication();

    // event.handling (optional override if custom app-level events needed)
    void OnEvent(CHEngine::Event &e) override;

private:
    CHEngine::EditorLayer *m_EditorLayer = nullptr;
};

#endif // CD_EDITOR_EDITOR_APPLICATION_H
