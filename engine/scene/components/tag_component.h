#ifndef CH_TAG_COMPONENT_H
#define CH_TAG_COMPONENT_H

#include <string>
#include "engine/scene/reflect.h"

namespace CHEngine
{
struct TagComponent
{
    std::string Tag;

    TagComponent() = default;
    TagComponent(const TagComponent &) = default;
    TagComponent(const std::string &tag) : Tag(tag)
    {
    }
};

BEGIN_REFLECT(TagComponent)
    PROPERTY(std::string, Tag, "Tag")
END_REFLECT()
} // namespace CHEngine

#endif // CH_TAG_COMPONENT_H
