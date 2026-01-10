#ifndef CH_PHYSICS_BVH_H
#define CH_PHYSICS_BVH_H

#include "engine/core/base.h"
#include "engine/core/thread_pool.h"
#include <future>
#include <raylib.h>
#include <raymath.h>
#include <vector>

namespace CH
{
struct CollisionTriangle
{
    Vector3 v0, v1, v2;
    Vector3 min, max;
    Vector3 center;

    CollisionTriangle(const Vector3 &a, const Vector3 &b, const Vector3 &c);
    bool IntersectsRay(const Ray &ray, float &t) const;
};

struct BVHNode
{
    Vector3 min;
    Vector3 max;
    std::vector<CollisionTriangle> triangles;
    Scope<BVHNode> left;
    Scope<BVHNode> right;

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
    static Scope<BVHNode> Build(const Model &model, const Matrix &transform = MatrixIdentity());

    // Asynchronous API
    static std::future<Scope<BVHNode>> BuildAsync(const Model &model,
                                                  const Matrix &transform = MatrixIdentity());

    static bool Raycast(const BVHNode *node, const Ray &ray, float &t, Vector3 &normal);

    // Thread pool access
    static ThreadPool &GetThreadPool();

private:
    static Scope<BVHNode> BuildRecursive(std::vector<CollisionTriangle> &tris, int depth);
    static bool RayInternal(const BVHNode *node, const Ray &ray, float &t, Vector3 &normal);
};
} // namespace CH

#endif // CH_BVH_H
