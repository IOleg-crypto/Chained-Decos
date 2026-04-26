#ifndef CH_PHYSICS_BVH_H
#define CH_PHYSICS_BVH_H

#include <vector>
#include <memory>
#include <glm/glm.hpp>

#include "bvh_node.h"
#include "engine/physics/collision/collision_triangle.h"
#include "engine/core/ch_math.h"

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

struct WorkItem
{
    uint32_t nodeIdx;
    size_t triStart;
    size_t triCount;
};

class BVH
{
public:
    BVH() = default;

    static std::shared_ptr<BVH> Build(std::vector<CollisionTriangle>&& triangles);

    bool Raycast(const Ray& ray, float& t, glm::vec3& normal, int& meshIndex) const;
    bool IntersectAABB(const BoundingBox& box, glm::vec3& outOverlapNormal, float& outOverlapDepth) const;

    struct BVHContact
    {
        glm::vec3 worldNormal;
        float depth;
        int triIndexA;
        int triIndexB;
    };

    void IntersectBVH(const BVH& other, const glm::mat4& matAToB, std::vector<BVHContact>& outContacts) const;
    void Refit(const std::vector<glm::vec3>& newVertices);

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

    static bool TestAxis(const glm::vec3& axis, const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2,
                        const glm::vec3& boxCenter, const glm::vec3& boxHalfSize);
    static bool TriangleIntersectAABB(const CollisionTriangle& tri, const BoundingBox& box);

private:
    std::vector<BVHNode> m_Nodes;
    std::vector<CollisionTriangle> m_Triangles;
};
} // namespace CHEngine

#endif // CH_PHYSICS_BVH_H
