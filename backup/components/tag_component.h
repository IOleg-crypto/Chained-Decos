#ifndef CH_TAG_COMPONENT_H
#define CH_TAG_COMPONENT_H

#include <string>

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

    static const char* GetStaticName() { return "TagComponent"; }

    template <typename Archive>
    static void Serialize(Archive& archive, TagComponent& component)
    {
        archive.Property("Tag", component.Tag);
    }
};

} // namespace CHEngine

#endif // CH_TAG_COMPONENT_H
