#ifndef CORE_I_SERVICE_H
#define CORE_I_SERVICE_H

namespace Core
{

// Base interface for all services (DI pattern)
class IService
{
public:
    virtual ~IService() = default;

    virtual void Initialize() = 0;
    virtual void Shutdown() = 0;
};

} // namespace Core

#endif // CORE_I_SERVICE_H
