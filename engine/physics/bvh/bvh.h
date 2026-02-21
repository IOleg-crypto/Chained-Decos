#ifndef CH_PHYSICS_BVH_H
#define CH_PHYSICS_BVH_H

#include "engine/core/base.h"
#include "raylib.h"
#include "raymath.h"
#include <future>
#include <vector>

#include "bvh_node.h"
#include "engine/physics/collision/collision_triangle.h"

namespace CHEngine
{

struct BuildContext
{
    std::vector<CollisionTriangle>& AllTriangles;
    std::vector<uint32_t> TriIndices;

    BuildContext(std::vector<CollisionTriangle>& tris)
        : AllTriangles(tris)
    {
        TriIndices.resize(tris.size());
        for (uint32_t i = 0; i < tris.size(); ++i)
        {
            TriIndices[i] = i;
        }
    }
};

class BVH
{
public:
    BVH() = default;

    // Snapshot of geometry data for thread-safe async building
    struct BVHMeshSnapshot
    {
        std::vector<Vector3> Vertices;
        std::vector<uint32_t> Indices;
        int MeshIndex = 0;
        Matrix Transform = MatrixIdentity();
    };

    struct BVHModelSnapshot
    {
        std::vector<BVHMeshSnapshot> Meshes;
    };

    // Synchronous API
    static std::shared_ptr<BVH> Build(const BVHModelSnapshot& model);
    static std::shared_ptr<BVH> Build(const Model& model, const Matrix& transform = MatrixIdentity());
    static std::shared_ptr<BVH> Build(const Model& model, const std::vector<Matrix>& offsetMatrices,
                                      const std::vector<int>& meshToNode, const Matrix& transform = MatrixIdentity());
    static std::shared_ptr<BVH> Build(std::shared_ptr<class ModelAsset> asset,
                                      const Matrix& transform = MatrixIdentity());

    // Asynchronous API
    static std::future<std::shared_ptr<BVH>> BuildAsync(const Model& model, const Matrix& transform = MatrixIdentity());
    static std::future<std::shared_ptr<BVH>> BuildAsync(const Model& model, const std::vector<Matrix>& offsetMatrices,
                                                        const std::vector<int>& meshToNode,
                                                        const Matrix& transform = MatrixIdentity());
    static std::future<std::shared_ptr<BVH>> BuildAsync(std::shared_ptr<class ModelAsset> asset,
                                                        const Matrix& transform = MatrixIdentity());

    bool Raycast(const Ray& ray, float& t, Vector3& normal, int& meshIndex) const;
    bool IntersectAABB(const BoundingBox& box, Vector3& outOverlapNormal, float& outOverlapDepth) const;

    // Returns triangles that intersect the AABB
    void QueryAABB(const BoundingBox& box, std::vector<const CollisionTriangle*>& outTriangles) const;

    const std::vector<BVHNode>& GetNodes() const
    {
        return m_Nodes;
    }
    const std::vector<CollisionTriangle>& GetTriangles() const
    {
        return m_Triangles;
    }

private:
    void BuildIterative(BuildContext& ctx, size_t totalTriCount);

    static bool TestAxis(const Vector3& axis, const Vector3& v0, const Vector3& v1, const Vector3& v2,
                        const Vector3& boxCenter, const Vector3& boxHalfSize);
    static bool TriangleIntersectAABB(const CollisionTriangle& tri, const BoundingBox& box);

private:
    std::vector<BVHNode> m_Nodes;
    std::vector<CollisionTriangle> m_Triangles;
};
} // namespace CHEngine

#endif // CH_BVH_H
