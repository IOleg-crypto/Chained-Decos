#ifndef CH_TAG_COMPONENT_H
#define CH_TAG_COMPONENT_H

#include "engine/core/reflection.h"

namespace CHEngine
{
struct TagComponent
{
    std::string Tag;

    TagComponent() = default;
    TagComponent(const TagComponent&) = default;
    TagComponent(const std::string& tag)
        : Tag(tag)
    {
    }

    CH_REFLECT_BEGIN(TagComponent)
        props.Property("Tag", Tag);
    CH_REFLECT_END()
};

} // namespace CHEngine

#endif // CH_TAG_COMPONENT_H
