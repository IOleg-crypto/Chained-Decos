#ifndef IENGINEMODULE_H
#define IENGINEMODULE_H

#include "IModule.h"
#include <string>
#include <vector>

namespace CHEngine
{
/**
 * @brief Interface for engine modules.
 *
 * This class provides a standardized interface for modules that integrate with the engine.
 * With the removal of the legacy Engine singleton, modules now interact through static subsystems.
 */
class IEngineModule : public IModule
{
public:
    virtual ~IEngineModule() = default;

    // IModule interface - implemented as adapters
    bool Initialize() override final
    {
        return true;
    }

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
        return GetModuleName();
    }

    // IEngineModule specific interface
    virtual const char *GetModuleName() const = 0;
    virtual const char *GetModuleVersion() const = 0;
    virtual const char *GetModuleDescription() const
    {
        return "";
    }

    // Standardized initialization (static subsystems are assumed to be ready)
    virtual bool InitializeModule() = 0;

    virtual void RegisterEvents()
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
} // namespace CHEngine

#endif // IENGINEMODULE_H
