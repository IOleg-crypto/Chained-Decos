#include "bvh.h"
#include <algorithm>
#include <cfloat>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace CHEngine
{

static bool RayAABBTest(const Ray& ray, const BoundingBox& box, float& t)
{
    glm::vec3 invDir = 1.0f / ray.direction;
    glm::vec3 t0 = (box.Min - ray.position) * invDir;
    glm::vec3 t1 = (box.Max - ray.position) * invDir;

    glm::vec3 tMin = glm::min(t0, t1);
    glm::vec3 tMax = glm::max(t0, t1);

    float minVal = std::max(std::max(tMin.x, tMin.y), tMin.z);
    float maxVal = std::min(std::min(tMax.x, tMax.y), tMax.z);

    if (maxVal >= std::max(0.0f, minVal))
    {
        t = minVal;
        return true;
    }
    return false;
}

std::shared_ptr<BVH> BVH::Build(std::vector<CollisionTriangle>&& triangles)
{
    if (triangles.empty()) return nullptr;

    auto bvh = std::make_shared<BVH>();
    bvh->m_Triangles = std::move(triangles);
    
    bvh->m_Nodes.reserve(bvh->m_Triangles.size() * 2);
    bvh->m_Nodes.emplace_back(); // Root

    BuildContext ctx(bvh->m_Triangles);
    bvh->BuildIterative(ctx, bvh->m_Triangles.size());

    std::vector<CollisionTriangle> reorderedTris;
    reorderedTris.reserve(bvh->m_Triangles.size());
    for (uint32_t idx : ctx.TriIndices)
    {
        reorderedTris.push_back(bvh->m_Triangles[idx]);
    }
    bvh->m_Triangles = std::move(reorderedTris);

    return bvh;
}

void BVH::BuildIterative(BuildContext& ctx, size_t totalTriCount)
{
    m_Nodes.clear();
    m_Nodes.reserve(totalTriCount * 2); 
    m_Nodes.emplace_back(); // Root (index 0)

    std::vector<WorkItem> stack;
    stack.push_back({0, 0, totalTriCount});

    while (!stack.empty())
    {
        auto [nodeIdx, triStart, triCount] = stack.back();
        stack.pop_back();

        glm::vec3 nodeMin = {FLT_MAX, FLT_MAX, FLT_MAX};
        glm::vec3 nodeMax = {-FLT_MAX, -FLT_MAX, -FLT_MAX};
        glm::vec3 cMin = {FLT_MAX, FLT_MAX, FLT_MAX};
        glm::vec3 cMax = {-FLT_MAX, -FLT_MAX, -FLT_MAX};

        for (size_t i = 0; i < triCount; ++i)
        {
            const auto& tri = ctx.AllTriangles[ctx.TriIndices[triStart + i]];
            nodeMin = glm::min(nodeMin, tri.min);
            nodeMax = glm::max(nodeMax, tri.max);
            cMin = glm::min(cMin, tri.center);
            cMax = glm::max(cMax, tri.center);
        }

        m_Nodes[nodeIdx].Min = nodeMin;
        m_Nodes[nodeIdx].Max = nodeMax;

        if (triCount <= 4)
        {
            m_Nodes[nodeIdx].LeftOrFirst = (uint32_t)triStart;
            m_Nodes[nodeIdx].TriangleCount = (uint16_t)triCount;
            continue;
        }

        glm::vec3 extent = cMax - cMin;
        int axis = 0;
        if (extent.y > extent.x && extent.y > extent.z) axis = 1;
        else if (extent.z > extent.x && extent.z > extent.y) axis = 2;

        float splitPos = 0;
        if (axis == 0) splitPos = cMin.x + extent.x * 0.5f;
        else if (axis == 1) splitPos = cMin.y + extent.y * 0.5f;
        else splitPos = cMin.z + extent.z * 0.5f;

        auto startIt = ctx.TriIndices.begin() + triStart;
        auto endIt = startIt + triCount;
        
        auto midIt = std::partition(startIt, endIt, [&](uint32_t idx) {
            const auto& tri = ctx.AllTriangles[idx];
            float val = (axis == 0) ? tri.center.x : (axis == 1) ? tri.center.y : tri.center.z;
            return val < splitPos;
        });

        size_t leftCount = std::distance(startIt, midIt);

        if (leftCount == 0 || leftCount == triCount)
        {
            leftCount = triCount / 2;
            std::nth_element(startIt, startIt + leftCount, endIt, [&](uint32_t a, uint32_t b) {
                const auto& triA = ctx.AllTriangles[a];
                const auto& triB = ctx.AllTriangles[b];
                float valA = (axis == 0) ? triA.center.x : (axis == 1) ? triA.center.y : triA.center.z;
                float valB = (axis == 0) ? triB.center.x : (axis == 1) ? triB.center.y : triB.center.z;
                return valA < valB;
            });
        }

        uint32_t leftIdx = (uint32_t)m_Nodes.size();
        m_Nodes.emplace_back();
        m_Nodes.emplace_back();

        m_Nodes[nodeIdx].LeftOrFirst = leftIdx;
        m_Nodes[nodeIdx].TriangleCount = 0;
        m_Nodes[nodeIdx].Axis = (uint16_t)axis;

        stack.push_back({leftIdx + 1, triStart + leftCount, triCount - leftCount});
        stack.push_back({leftIdx, triStart, leftCount});
    }
}

bool BVH::Raycast(const Ray& ray, float& t, glm::vec3& normal, int& meshIndex) const
{
    if (m_Nodes.empty()) return false;

    uint32_t stack[64];
    uint32_t stackPtr = 0;
    stack[stackPtr++] = 0;

    bool hit = false;
    while (stackPtr > 0)
    {
        const BVHNode& node = m_Nodes[stack[--stackPtr]];

        float boxT = 0;
        if (!RayAABBTest(ray, {node.Min, node.Max}, boxT) || boxT >= t) continue;

        if (node.IsLeaf())
        {
            for (uint32_t i = 0; i < node.TriangleCount; ++i)
            {
                const auto& tri = m_Triangles[node.LeftOrFirst + i];
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
            uint32_t left = node.LeftOrFirst;
            uint32_t right = left + 1;

            float leftT = FLT_MAX, rightT = FLT_MAX;
            bool leftValid = RayAABBTest(ray, {m_Nodes[left].Min, m_Nodes[left].Max}, leftT) && leftT < t;
            bool rightValid = RayAABBTest(ray, {m_Nodes[right].Min, m_Nodes[right].Max}, rightT) && rightT < t;

            if (leftValid && rightValid)
            {
                if (leftT < rightT)
                {
                    stack[stackPtr++] = right;
                    stack[stackPtr++] = left;
                }
                else
                {
                    stack[stackPtr++] = left;
                    stack[stackPtr++] = right;
                }
            }
            else if (leftValid) stack[stackPtr++] = left;
            else if (rightValid) stack[stackPtr++] = right;
        }
    }
    return hit;
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

        if (!(node.Min.x <= box.Max.x && node.Max.x >= box.Min.x && node.Min.y <= box.Max.y &&
              node.Max.y >= box.Min.y && node.Min.z <= box.Max.z && node.Max.z >= box.Min.z))
        {
            continue;
        }

        if (node.IsLeaf())
        {
            for (uint32_t i = 0; i < node.TriangleCount; ++i)
            {
                const auto& tri = m_Triangles[node.LeftOrFirst + i];
                if (BVH::TriangleIntersectAABB(tri, box))
                {
                    glm::vec3 triNormal = tri.normal;
                    glm::vec3 boxCenter = (box.Min + box.Max) * 0.5f;
                    float dist = glm::dot(tri.v0 - boxCenter, triNormal);
                    float radius = 0.5f * (std::abs(triNormal.x * (box.Max.x - box.Min.x)) +
                                           std::abs(triNormal.y * (box.Max.y - box.Min.y)) +
                                           std::abs(triNormal.z * (box.Max.z - box.Min.z)));

                    float depth = radius - std::abs(dist);
                    if (depth > outDepth)
                    {
                        outDepth = depth;
                        outNormal = (dist > 0) ? -triNormal : triNormal;
                        hit = true;
                    }
                }
            }
        }
        else
        {
            stack[stackPtr++] = node.LeftOrFirst;
            stack[stackPtr++] = node.LeftOrFirst + 1;
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

        if (!(node.Min.x <= box.Max.x && node.Max.x >= box.Min.x && node.Min.y <= box.Max.y &&
              node.Max.y >= box.Min.y && node.Min.z <= box.Max.z && node.Max.z >= box.Min.z))
        {
            continue;
        }

        if (node.IsLeaf())
        {
            for (uint32_t i = 0; i < node.TriangleCount; ++i)
            {
                const auto& tri = m_Triangles[node.LeftOrFirst + i];
                if (BVH::TriangleIntersectAABB(tri, box))
                {
                    outTriangles.push_back(&tri);
                }
            }
        }
        else
        {
            stack[stackPtr++] = node.LeftOrFirst;
            stack[stackPtr++] = node.LeftOrFirst + 1;
        }
    }
}

void BVH::Refit(const std::vector<glm::vec3>& newVertices)
{
    // Update triangles with new vertex positions (if the triangle indices match)
    // This assumes the order of triangles and vertices hasn't changed.
    // We update in-place and then propagate bounding boxes up.
    if (m_Nodes.empty()) return;

    // First pass: Update leaf node bounding boxes from updated triangles
    for (int i = (int)m_Nodes.size() - 1; i >= 0; i--)
    {
        BVHNode& node = m_Nodes[i];
        if (node.IsLeaf())
        {
            glm::vec3 nodeMin = {FLT_MAX, FLT_MAX, FLT_MAX};
            glm::vec3 nodeMax = {-FLT_MAX, -FLT_MAX, -FLT_MAX};
            for (uint32_t j = 0; j < node.TriangleCount; j++)
            {
                CollisionTriangle& tri = m_Triangles[node.LeftOrFirst + j];
                // Note: v0,v1,v2 are world/local cached. We might need a way to relate newVertices to tri indices.
                // For now, we assume simple mapping exists if caller provides it.
                // In a real skeletal system, this would be more complex.
                nodeMin = glm::min(nodeMin, tri.min);
                nodeMax = glm::max(nodeMax, tri.max);
            }
            node.Min = nodeMin;
            node.Max = nodeMax;
        }
        else
        {
            const BVHNode& left = m_Nodes[node.LeftOrFirst];
            const BVHNode& right = m_Nodes[node.LeftOrFirst + 1];
            node.Min = glm::min(left.Min, right.Min);
            node.Max = glm::max(left.Max, right.Max);
        }
    }
}

static bool IntersectNodeNode(const BVH& bvhA, uint32_t idxA, const BVH& bvhB, uint32_t idxB, const glm::mat4& matAToB, 
                              std::vector<BVH::BVHContact>& outContacts)
{
    const BVHNode& nodeA = bvhA.GetNodes()[idxA];
    const BVHNode& nodeB = bvhB.GetNodes()[idxB];

    // Transform nodeA AABB into space of B for overlap check
    // Optimization: avoid full 8-corner transform if AABB-AABB is enough
    // For now, rough check
    if (!(nodeA.Min.x <= 1e20f)) return false; // Safety

    // ... BVH-BVH recursion logic ...
    // To implement THIS fully we need to be very careful with stack depth on recursion
    // A better way is using a stack of pairs
    return false; // Stub for now, will implement full recursion in next step
}

void BVH::IntersectBVH(const BVH& other, const glm::mat4& matAToB, std::vector<BVHContact>& outContacts) const
{
    if (m_Nodes.empty() || other.m_Nodes.empty()) return;

    struct Pair { uint32_t a, b; };
    Pair stack[128];
    int top = 0;
    stack[top++] = {0, 0};

    // Transform A's nodes to B's space? No, build a local BBox for each check.
    while (top > 0)
    {
        Pair curr = stack[--top];
        const BVHNode& nodeA = m_Nodes[curr.a];
        const BVHNode& nodeB = other.m_Nodes[curr.b];

        // 1. Overlap test between nodeA (in world) and nodeB (in world)
        // matAToB transforms from A's local space to B's local space.
        
        // Transform A's AABB to B's space
        glm::vec3 corners[8] = {
            {nodeA.Min.x, nodeA.Min.y, nodeA.Min.z}, {nodeA.Max.x, nodeA.Min.y, nodeA.Min.z},
            {nodeA.Min.x, nodeA.Max.y, nodeA.Min.z}, {nodeA.Max.x, nodeA.Max.y, nodeA.Min.z},
            {nodeA.Min.x, nodeA.Min.y, nodeA.Max.z}, {nodeA.Max.x, nodeA.Min.y, nodeA.Max.z},
            {nodeA.Min.x, nodeA.Max.y, nodeA.Max.z}, {nodeA.Max.x, nodeA.Max.y, nodeA.Max.z}};
        
        BoundingBox aInB = {{1e30f, 1e30f, 1e30f}, {-1e30f, -1e30f, -1e30f}};
        for(int i=0; i<8; i++) {
            glm::vec3 p = glm::vec3(matAToB * glm::vec4(corners[i], 1.0f));
            aInB.Min = glm::min(aInB.Min, p);
            aInB.Max = glm::max(aInB.Max, p);
        }

        if (!(aInB.Min.x <= nodeB.Max.x && aInB.Max.x >= nodeB.Min.x &&
              aInB.Min.y <= nodeB.Max.y && aInB.Max.y >= nodeB.Min.y &&
              aInB.Min.z <= nodeB.Max.z && aInB.Max.z >= nodeB.Min.z))
        {
            continue;
        }

        if (nodeA.IsLeaf() && nodeB.IsLeaf())
        {
            for (uint32_t i = 0; i < nodeA.TriangleCount; i++)
            {
                const auto& triA = m_Triangles[nodeA.LeftOrFirst + i];
                glm::vec3 v0 = glm::vec3(matAToB * glm::vec4(triA.v0, 1.0f));
                glm::vec3 v1 = glm::vec3(matAToB * glm::vec4(triA.v1, 1.0f));
                glm::vec3 v2 = glm::vec3(matAToB * glm::vec4(triA.v2, 1.0f));
                
                BoundingBox triABBox = {glm::min(v0, glm::min(v1, v2)), glm::max(v0, glm::max(v1, v2))};

                for (uint32_t j = 0; j < nodeB.TriangleCount; j++)
                {
                    const auto& triB = other.m_Triangles[nodeB.LeftOrFirst + j];
                    if (BVH::TriangleIntersectAABB(triB, triABBox))
                    {
                         // Exact Triangle-Triangle test would be here. 
                         // For now, use the AABB depth as a proxy contact.
                         // (Will refine to actual Triangle-Triangle SAT later)
                         glm::vec3 normal = triB.normal;
                         float dist = glm::dot(v0 - triB.v0, normal);
                         if (dist < 0.0f) {
                             outContacts.push_back({normal, -dist, (int)(nodeA.LeftOrFirst + i), (int)(nodeB.LeftOrFirst + j)});
                         }
                    }
                }
            }
        }
        else if (nodeA.IsLeaf())
        {
            stack[top++] = {curr.a, nodeB.LeftOrFirst};
            stack[top++] = {curr.a, nodeB.LeftOrFirst + 1};
        }
        else if (nodeB.IsLeaf())
        {
            stack[top++] = {nodeA.LeftOrFirst, curr.b};
            stack[top++] = {nodeA.LeftOrFirst + 1, curr.b};
        }
        else
        {
            // Traverse larger volume first
            float volA = (nodeA.Max.x - nodeA.Min.x) * (nodeA.Max.y - nodeA.Min.y) * (nodeA.Max.z - nodeA.Min.z);
            float volB = (nodeB.Max.x - nodeB.Min.x) * (nodeB.Max.y - nodeB.Min.y) * (nodeB.Max.z - nodeB.Min.z);
            if (volA > volB)
            {
                stack[top++] = {nodeA.LeftOrFirst, curr.b};
                stack[top++] = {nodeA.LeftOrFirst + 1, curr.b};
            }
            else
            {
                stack[top++] = {curr.a, nodeB.LeftOrFirst};
                stack[top++] = {curr.a, nodeB.LeftOrFirst + 1};
            }
        }
    }
}

} // namespace CHEngine