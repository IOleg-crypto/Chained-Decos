#ifndef CH_PHYSICS_BVH_H
#define CH_PHYSICS_BVH_H

#include "engine/core/base.h"
#include <future>
#include <raylib.h>
#include <raymath.h>
#include <vector>

namespace CHEngine
{
struct CollisionTriangle
{
    Vector3 v0, v1, v2;
    Vector3 min, max;
    Vector3 center;
    int meshIndex = -1;

    CollisionTriangle(const Vector3 &a, const Vector3 &b, const Vector3 &c, int index = -1);
    bool IntersectsRay(const Ray &ray, float &t) const;
};

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
    static Ref<BVHNode> BuildRecursive(std::vector<CollisionTriangle> &tris, int depth);
    static bool RayInternal(const BVHNode *node, const Ray &ray, float &t, Vector3 &normal,
                            int &meshIndex);
};
} // namespace CHEngine

#endif // CH_BVH_H
