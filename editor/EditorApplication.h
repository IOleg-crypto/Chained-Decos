#ifndef EDITOR_APPLICATION_H
#define EDITOR_APPLICATION_H

#include "core/application/Application.h"

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

    // Event handling (optional override if custom app-level events needed)
    void OnEvent(CHEngine::Event &e) override;

private:
    CHEngine::EditorLayer *m_EditorLayer = nullptr;
};

#endif // EDITOR_APPLICATION_H
