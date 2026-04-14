#ifndef CH_BVH_NODE_H
#define CH_BVH_NODE_H

#include <cstdint>
#include <glm/glm.hpp>

namespace CHEngine
{
struct BVHNode
{
    glm::vec3 Min;
    glm::vec3 Max;
    uint32_t LeftOrFirst; // Index of left child or first triangle
    uint32_t TriangleCount;
    uint32_t Axis; // 0=X, 1=Y, 2=Z for internal nodes

    bool IsLeaf() const
    {
        return TriangleCount > 0;
    }
};
} // namespace CHEngine

#endif // CH_BVH_NODE_H
