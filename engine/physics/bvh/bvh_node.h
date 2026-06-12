#ifndef CH_BVH_NODE_H
#define CH_BVH_NODE_H

#include <cstdint>
#include <glm/glm.hpp>

namespace Chained
{
struct alignas(32) BVHNode
{
    glm::vec3 AABBMin;
    uint32_t LeftFirst;   // Index of left child (internal) OR first primitive index (leaf)
    glm::vec3 AABBMax;
    uint32_t PrimCount;   // Number of primitives (0 = internal node)

    bool IsLeaf() const
    {
        return PrimCount > 0;
    }
};
} // namespace Chained

#endif // CH_BVH_NODE_H
