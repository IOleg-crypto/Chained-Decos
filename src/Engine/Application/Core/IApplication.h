#ifndef I_APPLICATION_H
#define I_APPLICATION_H

#include <string>

class Kernel;
class ModuleManager;
class Engine;

// Interface for all applications using the engine
// Implement this interface to define your application's behavior
class IApplication
{
public:
    virtual ~IApplication() = default;

    // Called before Engine initialization
    // Use this to configure the application (window size, title, etc.)
    virtual void OnPreInitialize()
    {
    }

    // Called after creating Engine, Kernel, but before Init()
    // Use this to initialize your own services that don't depend on full engine init
    virtual void OnInitializeServices()
    {
    }

    // Called after registering Engine services
    // Use this to register your own services into the Kernel
    virtual void OnRegisterEngineServices()
    {
    }

    // Called to register project modules (REQUIRED)
    // Use this to register your game/editor modules
    virtual void OnRegisterProjectModules() = 0;

    // Called to register project services
    // Use this to register services that depend on modules
    virtual void OnRegisterProjectServices()
    {
    }

    // Called before initializing all modules
    virtual void OnPreInitializeModules()
    {
    }

    // Called after full initialization
    // The engine is fully ready at this point
    virtual void OnPostInitialize()
    {
    }

    // Called before each frame update
    virtual void OnPreUpdate(float deltaTime)
    {
    }

    // Called after updating Engine and modules
    virtual void OnPostUpdate(float deltaTime)
    {
    }

    // Called before rendering
    virtual void OnPreRender()
    {
    }

    // Called after rendering Engine and modules
    virtual void OnPostRender()
    {
    }

    // Called before shutdown
    virtual void OnPreShutdown()
    {
    }

    // Called for command line processing (optional)
    virtual void ProcessCommandLine(int argc, char *argv[])
    {
    }

    // Configure kernel before initialization
    virtual void ConfigureKernel(Kernel *kernel)
    {
    }

    // Configure module manager
    virtual void ConfigureModuleManager(ModuleManager *manager)
    {
    }

    // Setters for core systems (called by EngineApplication)
    virtual void SetEngine(Engine *engine)
    {
        m_engine = engine;
    }
    virtual void SetKernel(Kernel *kernel)
    {
        m_kernel = kernel;
    }

protected:
    Engine *GetEngine() const
    {
        return m_engine;
    }
    Kernel *GetKernel() const
    {
        return m_kernel;
    }

private:
    Engine *m_engine = nullptr;
    Kernel *m_kernel = nullptr;
};

#endif // I_APPLICATION_H
