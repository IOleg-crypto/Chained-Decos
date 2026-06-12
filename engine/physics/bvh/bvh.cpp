#include "bvh.h"
#include "engine/physics/collision_core.h"
#include <algorithm>
#include <cfloat>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Chained
{

std::shared_ptr<BVH> BVH::Build(std::vector<CollisionTriangle>&& triangles)
{
    if (triangles.empty()) return nullptr;

    auto bvh = std::make_shared<BVH>();
    bvh->m_Triangles = std::move(triangles);
    
    uint32_t N = (uint32_t)bvh->m_Triangles.size();
    bvh->m_PrimitiveIndices.resize(N);
    for (uint32_t i = 0; i < N; i++)
    {
        bvh->m_PrimitiveIndices[i] = i;
    }

    bvh->m_Nodes.resize(N * 2 - 1);
    bvh->m_NodesUsed = 1;

    BVHNode& root = bvh->m_Nodes[0];
    root.LeftFirst = 0;
    root.PrimCount = N;

    bvh->UpdateNodeBounds(0);
    bvh->Subdivide(0);

    return bvh;
}

void BVH::UpdateNodeBounds(uint32_t nodeIdx)
{
    BVHNode& node = m_Nodes[nodeIdx];
    node.AABBMin = glm::vec3(FLT_MAX);
    node.AABBMax = glm::vec3(-FLT_MAX);

    for (uint32_t i = 0; i < node.PrimCount; i++)
    {
        uint32_t triIdx = m_PrimitiveIndices[node.LeftFirst + i];
        const auto& tri = m_Triangles[triIdx];
        node.AABBMin = glm::min(node.AABBMin, tri.min);
        node.AABBMax = glm::max(node.AABBMax, tri.max);
    }
}

void BVH::Subdivide(uint32_t nodeIdx)
{
    BVHNode& node = m_Nodes[nodeIdx];
    
    // Determine split axis based on largest extent
    glm::vec3 extent = node.AABBMax - node.AABBMin;
    int axis = 0;
    if (extent.y > extent.x && extent.y > extent.z) axis = 1;
    else if (extent.z > extent.x && extent.z > extent.y) axis = 2;

    float splitPos = node.AABBMin[axis] + extent[axis] * 0.5f;

    // Partition primitives
    int i = node.LeftFirst;
    int j = i + node.PrimCount - 1;
    while (i <= j)
    {
        uint32_t triIdx = m_PrimitiveIndices[i];
        if (m_Triangles[triIdx].center[axis] < splitPos)
            i++;
        else
            std::swap(m_PrimitiveIndices[i], m_PrimitiveIndices[j--]);
    }

    uint32_t leftCount = i - node.LeftFirst;
    if (leftCount == 0 || leftCount == node.PrimCount) return;

    // Create child nodes
    uint32_t leftChildIdx = m_NodesUsed++;
    uint32_t rightChildIdx = m_NodesUsed++;

    m_Nodes[leftChildIdx].LeftFirst = node.LeftFirst;
    m_Nodes[leftChildIdx].PrimCount = leftCount;

    m_Nodes[rightChildIdx].LeftFirst = i;
    m_Nodes[rightChildIdx].PrimCount = node.PrimCount - leftCount;

    node.LeftFirst = leftChildIdx;
    node.PrimCount = 0; // Internal node

    UpdateNodeBounds(leftChildIdx);
    UpdateNodeBounds(rightChildIdx);

    Subdivide(leftChildIdx);
    Subdivide(rightChildIdx);
}

static bool RayAABBTest(const Ray& ray, const glm::vec3 bmin, const glm::vec3 bmax, float& t)
{
    glm::vec3 invDir = 1.0f / (ray.direction + glm::vec3(1e-10f)); // Avoid direct division by zero
    glm::vec3 t0 = (bmin - ray.position) * invDir;
    glm::vec3 t1 = (bmax - ray.position) * invDir;
    
    glm::vec3 tmin = glm::min(t0, t1);
    glm::vec3 tmax = glm::max(t0, t1);
    
    float nearT = std::max({tmin.x, tmin.y, tmin.z});
    float farT = std::min({tmax.x, tmax.y, tmax.z});
    
    t = nearT;
    return farT >= nearT && farT > 0;
}

bool BVH::Raycast(const Ray& ray, float& t, glm::vec3& normal, int& meshIndex) const
{
    uint32_t stack[64];
    uint32_t stackPtr = 0;
    stack[stackPtr++] = 0;
    bool hit = false;

    while (stackPtr > 0)
    {
        uint32_t nodeIdx = stack[--stackPtr];
        const BVHNode& node = m_Nodes[nodeIdx];

        float dist;
        if (!RayAABBTest(ray, node.AABBMin, node.AABBMax, dist) || dist >= t) continue;

        if (node.IsLeaf())
        {
            for (uint32_t i = 0; i < node.PrimCount; i++)
            {
                const auto& tri = m_Triangles[m_PrimitiveIndices[node.LeftFirst + i]];
                float triT = t;
                glm::vec3 triNormal;
                if (tri.IntersectsRay(ray, triT, triNormal) && triT < t)
                {
                    t = triT;
                    normal = triNormal;
                    meshIndex = tri.meshIndex;
                    hit = true;
                }
            }
        }
        else
        {
            stack[stackPtr++] = node.LeftFirst + 1;
            stack[stackPtr++] = node.LeftFirst;
        }
    }
    return hit;
}

bool BVH::IntersectAABB(const BoundingBox& box, glm::vec3& outNormal, float& outDepth) const
{
    if (m_Nodes.empty()) return false;

    uint32_t stack[64];
    uint32_t stackPtr = 0;
    stack[stackPtr++] = 0;

    bool hit = false;
    while (stackPtr > 0)
    {
        const BVHNode& node = m_Nodes[stack[--stackPtr]];

        if (!(node.AABBMin.x <= box.Max.x && node.AABBMax.x >= box.Min.x && 
              node.AABBMin.y <= box.Max.y && node.AABBMax.y >= box.Min.y && 
              node.AABBMin.z <= box.Max.z && node.AABBMax.z >= box.Min.z))
        {
            continue;
        }

        if (node.IsLeaf())
        {
            for (uint32_t i = 0; i < node.PrimCount; ++i)
            {
                const auto& tri = m_Triangles[m_PrimitiveIndices[node.LeftFirst + i]];
                if (BVH::TriangleIntersectAABB(tri, box))
                {
                    glm::vec3 triNormal = tri.normal;
                    glm::vec3 boxCenter = (box.Min + box.Max) * 0.5f;
                    glm::vec3 boxHalfSize = (box.Max - box.Min) * 0.5f;

                    // Axis 1: Triangle Normal
                    float distNormal = glm::dot(tri.v0 - boxCenter, triNormal);
                    float radiusNormal = boxHalfSize.x * std::abs(triNormal.x) +
                                         boxHalfSize.y * std::abs(triNormal.y) +
                                         boxHalfSize.z * std::abs(triNormal.z);
                    float depthNormal = radiusNormal - std::abs(distNormal);

                    // Axes 2,3,4: Box Axes (AABB centered at boxCenter)
                    float triMinX = std::min({tri.v0.x - boxCenter.x, tri.v1.x - boxCenter.x, tri.v2.x - boxCenter.x});
                    float triMaxX = std::max({tri.v0.x - boxCenter.x, tri.v1.x - boxCenter.x, tri.v2.x - boxCenter.x});
                    float depthX = std::min(boxHalfSize.x - triMinX, triMaxX + boxHalfSize.x);

                    float triMinY = std::min({tri.v0.y - boxCenter.y, tri.v1.y - boxCenter.y, tri.v2.y - boxCenter.y});
                    float triMaxY = std::max({tri.v0.y - boxCenter.y, tri.v1.y - boxCenter.y, tri.v2.y - boxCenter.y});
                    float depthY = std::min(boxHalfSize.y - triMinY, triMaxY + boxHalfSize.y);

                    float triMinZ = std::min({tri.v0.z - boxCenter.z, tri.v1.z - boxCenter.z, tri.v2.z - boxCenter.z});
                    float triMaxZ = std::max({tri.v0.z - boxCenter.z, tri.v1.z - boxCenter.z, tri.v2.z - boxCenter.z});
                    float depthZ = std::min(boxHalfSize.z - triMinZ, triMaxZ + boxHalfSize.z);

                    // Min penetration
                    float minDepth = depthNormal;
                    glm::vec3 minNormal = (distNormal > 0) ? -triNormal : triNormal;

                    if (depthX < minDepth) { minDepth = depthX; minNormal = (triMaxX + triMinX > 0) ? glm::vec3(-1,0,0) : glm::vec3(1,0,0); }
                    if (depthY < minDepth) { minDepth = depthY; minNormal = (triMaxY + triMinY > 0) ? glm::vec3(0,-1,0) : glm::vec3(0,1,0); }
                    if (depthZ < minDepth) { minDepth = depthZ; minNormal = (triMaxZ + triMinZ > 0) ? glm::vec3(0,0,-1) : glm::vec3(0,0,1); }

                    if (minDepth > outDepth)
                    {
                        outDepth = minDepth;
                        outNormal = minNormal;
                        hit = true;
                    }
                }
            }
        }
        else
        {
            stack[stackPtr++] = node.LeftFirst;
            stack[stackPtr++] = node.LeftFirst + 1;
        }
    }
    return hit;
}

void BVH::QueryAABB(const BoundingBox& box, std::vector<const CollisionTriangle*>& outTriangles) const
{
    if (m_Nodes.empty()) return;

    uint32_t stack[64];
    uint32_t stackPtr = 0;
    stack[stackPtr++] = 0;

    while (stackPtr > 0)
    {
        const BVHNode& node = m_Nodes[stack[--stackPtr]];

        if (!(node.AABBMin.x <= box.Max.x && node.AABBMax.x >= box.Min.x && 
              node.AABBMin.y <= box.Max.y && node.AABBMax.y >= box.Min.y && 
              node.AABBMin.z <= box.Max.z && node.AABBMax.z >= box.Min.z))
        {
            continue;
        }

        if (node.IsLeaf())
        {
            for (uint32_t i = 0; i < node.PrimCount; ++i)
            {
                const auto& tri = m_Triangles[m_PrimitiveIndices[node.LeftFirst + i]];
                if (BVH::TriangleIntersectAABB(tri, box))
                {
                    outTriangles.push_back(&tri);
                }
            }
        }
        else
        {
            stack[stackPtr++] = node.LeftFirst;
            stack[stackPtr++] = node.LeftFirst + 1;
        }
    }
}

void BVH::Refit(const std::vector<glm::vec3>& newVertices)
{
    if (m_Nodes.empty()) return;

    // First pass: Update leaf node bounding boxes from updated triangles
    for (int i = (int)m_NodesUsed - 1; i >= 0; i--)
    {
        BVHNode& node = m_Nodes[i];
        if (node.IsLeaf())
        {
            UpdateNodeBounds(i);
        }
        else
        {
            const BVHNode& left = m_Nodes[node.LeftFirst];
            const BVHNode& right = m_Nodes[node.LeftFirst + 1];
            node.AABBMin = glm::min(left.AABBMin, right.AABBMin);
            node.AABBMax = glm::max(left.AABBMax, right.AABBMax);
        }
    }
}

void BVH::IntersectBVH(const BVH& other, const glm::mat4& matAToB, std::vector<BVHContact>& outContacts) const
{
    if (m_Nodes.empty() || other.m_Nodes.empty()) return;

    struct Pair { uint32_t a, b; };
    Pair stack[128];
    int top = 0;
    stack[top++] = {0, 0};

    while (top > 0)
    {
        if (top >= 127) { break; } 
        
        Pair curr = stack[--top];
        const BVHNode& nodeA = m_Nodes[curr.a];
        const BVHNode& nodeB = other.m_Nodes[curr.b];

        // Transform A's AABB to B's space for overlap check
        glm::vec3 corners[8] = {
            {nodeA.AABBMin.x, nodeA.AABBMin.y, nodeA.AABBMin.z}, {nodeA.AABBMax.x, nodeA.AABBMin.y, nodeA.AABBMin.z},
            {nodeA.AABBMin.x, nodeA.AABBMax.y, nodeA.AABBMin.z}, {nodeA.AABBMax.x, nodeA.AABBMax.y, nodeA.AABBMin.z},
            {nodeA.AABBMin.x, nodeA.AABBMin.y, nodeA.AABBMax.z}, {nodeA.AABBMax.x, nodeA.AABBMin.y, nodeA.AABBMax.z},
            {nodeA.AABBMin.x, nodeA.AABBMax.y, nodeA.AABBMax.z}, {nodeA.AABBMax.x, nodeA.AABBMax.y, nodeA.AABBMax.z}};
        
        BoundingBox aInB = {{1e30f, 1e30f, 1e30f}, {-1e30f, -1e30f, -1e30f}};
        for(int i=0; i<8; i++) {
            glm::vec3 p = glm::vec3(matAToB * glm::vec4(corners[i], 1.0f));
            aInB.Min = glm::min(aInB.Min, p);
            aInB.Max = glm::max(aInB.Max, p);
        }

        if (!(aInB.Min.x <= nodeB.AABBMax.x && aInB.Max.x >= nodeB.AABBMin.x &&
              aInB.Min.y <= nodeB.AABBMax.y && aInB.Max.y >= nodeB.AABBMin.y &&
              aInB.Min.z <= nodeB.AABBMax.z && aInB.Max.z >= nodeB.AABBMin.z))
        {
            continue;
        }

        if (nodeA.IsLeaf() && nodeB.IsLeaf())
        {
            for (uint32_t i = 0; i < nodeA.PrimCount; i++)
            {
                uint32_t triIdxA = m_PrimitiveIndices[nodeA.LeftFirst + i];
                const auto& triA = m_Triangles[triIdxA];
                glm::vec3 v0 = glm::vec3(matAToB * glm::vec4(triA.v0, 1.0f));
                glm::vec3 v1 = glm::vec3(matAToB * glm::vec4(triA.v1, 1.0f));
                glm::vec3 v2 = glm::vec3(matAToB * glm::vec4(triA.v2, 1.0f));
                
                BoundingBox triABBox = {glm::min(v0, glm::min(v1, v2)), glm::max(v0, glm::max(v1, v2))};

                for (uint32_t j = 0; j < nodeB.PrimCount; j++)
                {
                    uint32_t triIdxB = other.m_PrimitiveIndices[nodeB.LeftFirst + j];
                    const auto& triB = other.m_Triangles[triIdxB];
                    
                    if (BVH::TriangleIntersectAABB(triB, triABBox))
                    {
                        // Check all three vertices of triA against triB's plane
                        glm::vec3 vertsA[3] = { v0, v1, v2 };
                        glm::vec3 normal = triB.normal;
                        float maxDepth = 0.0f;
                        bool hit = false;

                        for(int k=0; k<3; k++)
                        {
                            float dist = glm::dot(vertsA[k] - triB.v0, normal);
                            if (dist < -0.001f) // Vertex is behind triangle B
                            {
                                float depth = -dist;
                                if (depth > maxDepth)
                                {
                                    maxDepth = depth;
                                    hit = true;
                                }
                            }
                        }

                        if (hit)
                        {
                            outContacts.push_back({normal, maxDepth, (int)triIdxA, (int)triIdxB});
                        }
                    }
                }
            }
        }
        else if (nodeA.IsLeaf())
        {
            stack[top++] = {curr.a, nodeB.LeftFirst};
            stack[top++] = {curr.a, nodeB.LeftFirst + 1};
        }
        else if (nodeB.IsLeaf())
        {
            stack[top++] = {nodeA.LeftFirst, curr.b};
            stack[top++] = {nodeA.LeftFirst + 1, curr.b};
        }
        else
        {
            stack[top++] = {nodeA.LeftFirst, curr.b};
            stack[top++] = {nodeA.LeftFirst + 1, curr.b};
        }
    }
}

bool BVH::TriangleIntersectAABB(const CollisionTriangle& tri, const BoundingBox& box)
{
    glm::vec3 boxCenter = (box.Min + box.Max) * 0.5f;
    glm::vec3 boxHalfSize = (box.Max - box.Min) * 0.5f;

    glm::vec3 v0 = tri.v0 - boxCenter;
    glm::vec3 v1 = tri.v1 - boxCenter;
    glm::vec3 v2 = tri.v2 - boxCenter;

    glm::vec3 e0 = v1 - v0;
    glm::vec3 e1 = v2 - v1;
    glm::vec3 e2 = v0 - v2;

    if (!BVH::TestAxis({1, 0, 0}, v0, v1, v2, {0, 0, 0}, boxHalfSize)) return false;
    if (!BVH::TestAxis({0, 1, 0}, v0, v1, v2, {0, 0, 0}, boxHalfSize)) return false;
    if (!BVH::TestAxis({0, 0, 1}, v0, v1, v2, {0, 0, 0}, boxHalfSize)) return false;

    glm::vec3 normal = glm::cross(e0, e1);
    if (!BVH::TestAxis(normal, v0, v1, v2, {0, 0, 0}, boxHalfSize)) return false;

    glm::vec3 axes[9] = {
        glm::cross(glm::vec3(1, 0, 0), e0), glm::cross(glm::vec3(1, 0, 0), e1), glm::cross(glm::vec3(1, 0, 0), e2),
        glm::cross(glm::vec3(0, 1, 0), e0), glm::cross(glm::vec3(0, 1, 0), e1), glm::cross(glm::vec3(0, 1, 0), e2),
        glm::cross(glm::vec3(0, 0, 1), e0), glm::cross(glm::vec3(0, 0, 1), e1), glm::cross(glm::vec3(0, 0, 1), e2)};

    for (int i = 0; i < 9; i++)
    {
        if (glm::length(axes[i]) < 0.0001f) continue;
        if (!BVH::TestAxis(axes[i], v0, v1, v2, {0, 0, 0}, boxHalfSize)) return false;
    }

    return true;
}

bool BVH::TestAxis(const glm::vec3& axis, const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2,
                   const glm::vec3& boxCenter, const glm::vec3& boxHalfSize)
{
    float p0 = glm::dot(v0, axis);
    float p1 = glm::dot(v1, axis);
    float p2 = glm::dot(v2, axis);

    float r = boxHalfSize.x * std::abs(axis.x) +
              boxHalfSize.y * std::abs(axis.y) +
              boxHalfSize.z * std::abs(axis.z);

    float triMin = std::min(std::min(p0, p1), p2);
    float triMax = std::max(std::max(p0, p1), p2);

    float boxProj = glm::dot(boxCenter, axis);
    float boxMin = boxProj - r;
    float boxMax = boxProj + r;

    return !(triMin > boxMax || triMax < boxMin);
}

} // namespace Chained