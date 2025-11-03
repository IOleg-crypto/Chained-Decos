#ifndef EDITOR_MODULE_H
#define EDITOR_MODULE_H

#include "Engine/Module/IEngineModule.h"
#include <memory>

// Forward declarations
class Kernel;
class Editor;
class CameraController;
class ModelLoader;

// Модуль для редактора карт
class EditorModule : public IEngineModule {
public:
    EditorModule();
    ~EditorModule() override = default;

    // IEngineModule interface
    const char* GetModuleName() const override { return "Editor"; }
    const char* GetModuleVersion() const override { return "1.0.0"; }
    const char* GetModuleDescription() const override { return "Map editor functionality"; }
    
    bool Initialize(Kernel* kernel) override;
    void Shutdown() override;
    
    void Update(float deltaTime) override;
    void Render() override;
    
    void RegisterServices(Kernel* kernel) override;
    std::vector<std::string> GetDependencies() const override;

    // Accessors
    Editor* GetEditor() const { return m_editor.get(); }

private:
    std::unique_ptr<Editor> m_editor;
    std::shared_ptr<CameraController> m_cameraController;
    std::unique_ptr<ModelLoader> m_modelLoader;
};

#endif // EDITOR_MODULE_H

