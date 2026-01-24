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
    Ref<BVHNode> left;
    Ref<BVHNode> right;

    BVHNode() = default;
    bool IsLeaf() const
    {
        return !left && !right;
    }
};
} // namespace CHEngine

#endif // CH_BVH_NODE_H
