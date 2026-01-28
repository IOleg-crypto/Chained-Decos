#ifndef CH_TAG_COMPONENT_H
#define CH_TAG_COMPONENT_H

#include "string"

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
} // namespace CHEngine

#endif // CH_TAG_COMPONENT_H
