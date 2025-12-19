#ifndef IMODULE_H
#define IMODULE_H

// Interface for modules
class IModule
{
public:
    virtual ~IModule() = default;

    // Module lifecycle
    virtual bool Initialize() = 0;
    virtual void Shutdown() = 0;
    virtual void Update(float deltaTime) = 0;
    virtual void Render() = 0;

    // Module identification
    virtual const char *GetName() const = 0;
};

#endif // IMODULE_H
