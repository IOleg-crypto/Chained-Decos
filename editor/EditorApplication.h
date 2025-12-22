#ifndef EDITOR_APPLICATION_H
#define EDITOR_APPLICATION_H

#include "Editor.h"
#include "core/application/IApplication.h"
#include <memory>

// Editor application - uses full engine + own modules
class EditorApplication : public ChainedDecos::IApplication
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
    void OnEvent(ChainedDecos::Event &e) override;

private:
    std::unique_ptr<Editor> m_editor;
};

#endif // EDITOR_APPLICATION_H
