#ifndef IENGINE_PROVIDER_H
#define IENGINE_PROVIDER_H

// Forward declaration
class Engine;

// Professional interface for accessing Engine
class IEngineProvider
{
public:
    virtual ~IEngineProvider() = default;
    virtual Engine* GetEngine() = 0;
};

#endif // IENGINE_PROVIDER_H

