#ifndef IENGINE_H
#define IENGINE_H

class RenderManager;
class InputManager;
class ModuleManager;

class IEngine
{
public:
    virtual ~IEngine() = default;
    
    virtual RenderManager* GetRenderManager() const = 0;
    virtual InputManager& GetInputManager() const = 0;
    virtual ModuleManager* GetModuleManager() = 0;
    virtual bool IsDebugInfoVisible() const = 0;
    virtual bool IsCollisionDebugVisible() const = 0;
};

#endif // IENGINE_H
