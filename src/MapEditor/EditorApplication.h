#ifndef EDITOR_APPLICATION_H
#define EDITOR_APPLICATION_H

#include "Engine/Application/EngineApplication.h"
#include <memory>

// Forward declaration
class Editor;

// Editor application - uses full engine + own modules
class EditorApplication : public EngineApplication {
public:
    EditorApplication();
    ~EditorApplication();

protected:
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

