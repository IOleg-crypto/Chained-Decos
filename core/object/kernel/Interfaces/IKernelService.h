#ifndef IKERNELSERVICE_H
#define IKERNELSERVICE_H

#include <string>

struct IKernelService
{
    virtual ~IKernelService() = default;
    virtual bool Initialize() = 0;
    virtual void Shutdown() = 0;
    virtual void Update(float /*deltaTime*/) {}
    virtual void Render() {}
    virtual const char *GetName() const = 0;
};

#endif // IKERNELSERVICE_H


