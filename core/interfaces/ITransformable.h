#ifndef CORE_ITRANSFORMABLE_H
#define CORE_ITRANSFORMABLE_H

#include <raylib.h>

namespace Core
{

class ITransformable
{
public:
    virtual ~ITransformable() = default;

    virtual Vector3 GetPosition() const = 0;
    virtual void SetPosition(const Vector3 &pos) = 0;

    virtual float GetRotationY() const = 0;
    virtual void SetRotationY(float rotation) = 0;
};

} // namespace Core

#endif // CORE_ITRANSFORMABLE_H
