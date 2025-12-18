#ifndef EDITOR_APPLICATION_H
#define EDITOR_APPLICATION_H

#include "Editor/Editor.h"
#include "core/engine/IApplication.h"
#include <memory>

// Editor application - uses full engine + own modules
class EditorApplication : public IApplication
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

    // Shutdown
    void OnShutdown() override;

private:
    std::unique_ptr<Editor> m_editor;
};

#endif // EDITOR_APPLICATION_H
