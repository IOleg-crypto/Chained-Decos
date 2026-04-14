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

} // namespace CHEngine