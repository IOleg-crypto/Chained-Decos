#ifndef EDITOR_MODULE_H
#define EDITOR_MODULE_H

#include "core/engine/Engine.h"
#include "core/object/module/Interfaces/IEngineModule.h"
#include <memory>
#include <string>
#include <vector>

// Editor module - minimal implementation
// Editor functionality is handled directly in EditorApplication
class EditorModule : public IEngineModule
{
public:
    EditorModule();
    ~EditorModule() override = default;

    // IEngineModule interface
    const char *GetModuleName() const override
    {
        return "Editor";
    }
    const char *GetModuleVersion() const override
    {
        return "1.0.0";
    }
    const char *GetModuleDescription() const override
    {
        return "Map editor functionality";
    }

    bool Initialize(Engine *engine) override;
    void Shutdown() override;

    void Update(float deltaTime) override;
    void Render() override;

    void RegisterServices(Engine *engine) override;
    std::vector<std::string> GetDependencies() const override;
};

#endif // EDITOR_MODULE_H
