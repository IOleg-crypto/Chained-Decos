#ifndef CH_ICONTROL_H
#define CH_ICONTROL_H

#include "engine/scene/entity.h"
#include <string>

namespace CHEngine
{
class IControl
{
public:
    virtual ~IControl() = default;
    virtual void Initialize(Entity entity) = 0;
    virtual const std::string &GetName() const = 0;
};
} // namespace CHEngine

#endif // CH_ICONTROL_H
