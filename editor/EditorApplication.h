#ifndef EDITOR_APPLICATION_H
#define EDITOR_APPLICATION_H

#include "core/application/IApplication.h"

namespace CHEngine
{
class EditorLayer;
}

// Editor application - uses full engine + own modules
class EditorApplication : public CHEngine::IApplication
{
public:
    EditorApplication(int argc, char *argv[]);
    ~EditorApplication();

    // 1. Configuration
    void OnConfigure(EngineConfig &config) override;

    // 2. Registration
    void OnRegister() override;

    // 3. Start
    void OnStart() override;

    // Update
    void OnUpdate(float deltaTime) override;

    // Render
    void OnRender() override;
    void OnImGuiRender() override;

    // Shutdown
    void OnShutdown() override;

    // Event handling
    void OnEvent(CHEngine::Event &e) override;

private:
    CHEngine::EditorLayer *m_EditorLayer = nullptr;
};

#endif // EDITOR_APPLICATION_H
