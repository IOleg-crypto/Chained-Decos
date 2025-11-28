#ifndef EDITOR_APPLICATION_H
#define EDITOR_APPLICATION_H

#include "editor/plugins/map_editor/Editor/Editor.h"
#include "platform/windows/Core/IApplication.h"
#include <memory>

// Editor application - uses full engine + own modules
class EditorApplication : public IApplication
{
public:
    EditorApplication();
    ~EditorApplication();

    // Settings before initialization
    void OnPreInitialize() override;

    // Initialize Editor components
    void OnInitializeServices() override;

    // Register Editor modules (REQUIRED)
    void OnRegisterProjectModules() override;

    // Register Editor services
    void OnRegisterProjectServices() override;

    // After initialization
    void OnPostInitialize() override;

    // Custom update logic
    void OnPostUpdate(float deltaTime) override;

    // Custom rendering logic
    void OnPostRender() override;

    // Before shutdown
    void OnPreShutdown() override;

private:
    std::unique_ptr<Editor> m_editor;
};

#endif // EDITOR_APPLICATION_H
