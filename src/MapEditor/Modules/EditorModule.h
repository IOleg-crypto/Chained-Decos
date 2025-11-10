#ifndef EDITOR_MODULE_H
#define EDITOR_MODULE_H

#include "Engine/Module/Interfaces/IEngineModule.h"
#include <memory>
#include <vector>
#include <string>

// Forward declarations
class Kernel;

// Editor module - minimal implementation
// Editor functionality is handled directly in EditorApplication
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
};

#endif // EDITOR_MODULE_H

