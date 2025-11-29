#ifndef IENGINEMODULE_H
#pragma once

#include "IModule.h"
#include <string>
#include <vector>

// Forward declaration
class Engine;

// Interface for engine modules with engine integration
// Extends IModule with engine-specific functionality
class IEngineModule : public IModule
{
public:
    virtual ~IEngineModule() = default;

    // IModule interface - implemented as adapters
    bool Initialize() override final
    {
        // Simple modules don't need engine
        return true;
    }

    // These are still overridable by derived classes
    void Shutdown() override
    {
    }
    void Update(float deltaTime) override
    {
    }
    void Render() override
    {
    }

    const char *GetName() const override final
    {
        // Delegate to GetModuleName
        return GetModuleName();
    }

    // IEngineModule specific interface - must be implemented by derived classes
    virtual const char *GetModuleName() const = 0;
    virtual const char *GetModuleVersion() const = 0;
    virtual const char *GetModuleDescription() const
    {
        return "";
    }

    // Engine-specific methods with engine parameter
    virtual bool Initialize(Engine *engine) = 0;
    virtual void RegisterServices(Engine *engine)
    {
    }
    virtual std::vector<std::string> GetDependencies() const
    {
        return {};
    }

    bool IsInitialized() const
    {
        return m_initialized;
    }
    void SetInitialized(bool value)
    {
        m_initialized = value;
    }

private:
    bool m_initialized = false;
};

#endif // IENGINEMODULE_H
