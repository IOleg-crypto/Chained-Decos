#ifndef CH_EDITOR_TYPES_H
#define CH_EDITOR_TYPES_H

#include <cstdint>

namespace CHEngine
{
enum class SceneState : uint8_t
{
    Edit = 0,
    Play = 1
};
} // namespace CHEngine

#endif // CH_EDITOR_TYPES_H
