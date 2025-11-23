#pragma once

// Interface for Engine access
class IEngine
{
public:
    virtual ~IEngine() = default;

    // Core functionality that external code might need
    virtual void *GetKernel() = 0;
    virtual void *GetModuleManager() = 0;
    virtual void *GetRenderManager() = 0;
    virtual void *GetInputManager() = 0;
};
