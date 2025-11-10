#ifndef IENGINEMODULE_H
#define IENGINEMODULE_H

#include <string>
#include <vector>

class Kernel;

class IEngineModule {
public:
    virtual ~IEngineModule() = default;
    
    virtual const char* GetModuleName() const = 0;
    virtual const char* GetModuleVersion() const = 0;
    virtual const char* GetModuleDescription() const { return ""; }
    
    virtual bool Initialize(Kernel* kernel) = 0;
    virtual void Shutdown() = 0;
    virtual void Update(float deltaTime) {}
    virtual void Render() {}
    virtual void RegisterServices(Kernel* kernel) {}
    virtual std::vector<std::string> GetDependencies() const { return {}; }
    
    bool IsInitialized() const { return m_initialized; }
    void SetInitialized(bool value) { m_initialized = value; }
    
protected:
    
private:
    bool m_initialized = false;
};

#endif // IENGINEMODULE_H

