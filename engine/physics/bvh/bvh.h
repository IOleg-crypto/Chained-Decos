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

    const std::vector<BVHNode>& GetNodes() const { return m_Nodes; }
    const std::vector<CollisionTriangle>& GetTriangles() const { return m_Triangles; }
    const std::vector<uint32_t>& GetPrimitiveIndices() const { return m_PrimitiveIndices; }

private:
    void UpdateNodeBounds(uint32_t nodeIdx);
    void Subdivide(uint32_t nodeIdx);

    static bool TestAxis(const glm::vec3& axis, const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2,
                        const glm::vec3& boxCenter, const glm::vec3& boxHalfSize);
    static bool TriangleIntersectAABB(const CollisionTriangle& tri, const BoundingBox& box);

private:
    std::vector<BVHNode> m_Nodes;
    std::vector<CollisionTriangle> m_Triangles;
    std::vector<uint32_t> m_PrimitiveIndices;
    uint32_t m_NodesUsed = 0;
};
} // namespace CHEngine

#endif // CH_PHYSICS_BVH_H
