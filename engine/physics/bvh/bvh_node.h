#ifndef CH_BVH_NODE_H
#define CH_BVH_NODE_H

#include "engine/core/base.h"
#include "engine/physics/collision/collision_triangle.h"
#include <vector>

namespace CHEngine
{
struct BVHNode
{
    Vector3 min;
    Vector3 max;
    std::vector<CollisionTriangle> triangles;
    std::shared_ptr<BVHNode> left;
    std::shared_ptr<BVHNode> right;

    // Two-Level support
    bool isSubBVH = false;
    int meshIndex = -1;

    BVHNode() = default;
    bool IsLeaf() const
    {
        return !left && !right;
    }
};
} // namespace CHEngine

#endif // CH_BVH_NODE_H
