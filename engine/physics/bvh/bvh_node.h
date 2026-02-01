#ifndef CH_BVH_NODE_H
#define CH_BVH_NODE_H

#include "engine/core/base.h"
#include "engine/physics/collision/collision_triangle.h"
#include <vector>

namespace CHEngine
{
struct BVHNode
{
    Vector3 Min;
    Vector3 Max;
    uint32_t LeftOrFirst; // Index of left child or first triangle
    uint16_t TriangleCount;
    uint16_t Axis; // 0=X, 1=Y, 2=Z for internal nodes

    bool IsLeaf() const
    {
        return TriangleCount > 0;
    }
};
} // namespace CHEngine

#endif // CH_BVH_NODE_H
