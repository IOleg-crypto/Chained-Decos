#ifndef CH_TAG_COMPONENT_H
#define CH_TAG_COMPONENT_H

#include "engine/core/reflection_rfl.h"

namespace CHEngine
{
struct TagComponent
{
    std::string Tag;
    static const char* GetStaticName() { return "TagComponent"; }
};

CH_MARK_RFL(TagComponent);

} // namespace CHEngine

#endif // CH_TAG_COMPONENT_H
