#ifndef CH_PHYSICS_BVH_H
#define CH_PHYSICS_BVH_H

#include "engine/core/base.h"
#include <future>
#include <raylib.h>
#include <raymath.h>
#include <vector>

#include "bvh_node.h"
#include "engine/physics/collision/collision_triangle.h"

namespace CHEngine
{
class BVHBuilder
{
public:
    // Synchronous API
    static Ref<BVHNode> Build(const Model &model, const Matrix &transform = MatrixIdentity());

    // Asynchronous API
    static std::future<Ref<BVHNode>> BuildAsync(const Model &model,
                                                const Matrix &transform = MatrixIdentity());

    static bool Raycast(const BVHNode *node, const Ray &ray, float &t, Vector3 &normal,
                        int &meshIndex);
    static bool IntersectAABB(const BVHNode *node, const BoundingBox &box,
                              Vector3 &outOverlapNormal, float &outOverlapDepth);

private:
    static Ref<BVHNode> BuildRecursive(std::vector<CollisionTriangle> &tris, size_t start,
                                       size_t end, int depth);
    static bool RayInternal(const BVHNode *node, const Ray &ray, float &t, Vector3 &normal,
                            int &meshIndex);
};
} // namespace CHEngine

#endif // CH_BVH_H
