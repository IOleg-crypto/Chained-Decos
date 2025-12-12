#ifndef EDITOR_APPLICATION_H
#define EDITOR_APPLICATION_H

#include "Editor/Editor.h"
#include "core/engine/IApplication.h"
#include <memory>

// Editor application - uses full engine + own modules
class EditorApplication : public IApplication
{
public:
    EditorApplication();
    ~EditorApplication();

    // IApplication interface - all methods receive Engine&
    void OnConfigure(EngineConfig &config) override;
    void OnRegister(Engine &engine) override;
    void OnStart(Engine &engine) override;
    void OnUpdate(float deltaTime, Engine &engine) override;
    void OnRender(Engine &engine) override;
    void OnShutdown() override;

private:
    std::unique_ptr<Editor> m_editor;
};

#endif // EDITOR_APPLICATION_H
