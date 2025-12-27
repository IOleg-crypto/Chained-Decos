#pragma once

#include <string>

namespace CHEngine
{

/**
 * TagComponent - Human-readable name for entities
 *
 * Used for debugging and editor display.
 */
struct TagComponent
{
    std::string Tag;

    TagComponent() = default;
    TagComponent(const std::string &tag) : Tag(tag)
    {
    }
    TagComponent(const TagComponent &) = default;

    operator std::string &()
    {
        return Tag;
    }
    operator const std::string &() const
    {
        return Tag;
    }
};

} // namespace CHEngine
