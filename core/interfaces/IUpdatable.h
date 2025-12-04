#ifndef CORE_IUPDATABLE_H
#define CORE_IUPDATABLE_H

namespace Core
{

class IUpdatable
{
public:
    virtual ~IUpdatable() = default;
    virtual void Update(float deltaTime) = 0;
};

} // namespace Core

#endif // CORE_IUPDATABLE_H
