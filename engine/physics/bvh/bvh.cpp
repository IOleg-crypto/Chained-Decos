#include "bvh.h"
#include "raymath.h"
#include <algorithm>
#include <cfloat>

namespace CHEngine
{

std::shared_ptr<BVH> BVH::Build(std::vector<CollisionTriangle>&& triangles)
{
    if (triangles.empty())
    {
        return nullptr;
    }

    auto bvh = std::make_shared<BVH>();
    bvh->m_Triangles = std::move(triangles);
    
    // Reserve nodes: worst case for binary BVH is 2N-1 nodes
    bvh->m_Nodes.reserve(bvh->m_Triangles.size() * 2);
    bvh->m_Nodes.emplace_back(); // Root

    BuildContext ctx(bvh->m_Triangles);
    bvh->BuildIterative(ctx, bvh->m_Triangles.size());

    // Reorder triangles based on serial indices to improve cache locality
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
    // 1. Резервуємо пам'ять, щоб уникнути реаллокацій та інвалідації
    m_Nodes.clear();
    m_Nodes.reserve(totalTriCount * 2); 
    m_Nodes.emplace_back(); // Корінь (index 0)

    std::vector<WorkItem> stack;
    stack.push_back({0, 0, totalTriCount});

    while (!stack.empty())
    {
        auto [nodeIdx, triStart, triCount] = stack.back();
        stack.pop_back();

        // Рахуємо AABB вузла та межі центроїдів
        Vector3 nodeMin = {FLT_MAX, FLT_MAX, FLT_MAX};
        Vector3 nodeMax = {-FLT_MAX, -FLT_MAX, -FLT_MAX};
        Vector3 cMin = {FLT_MAX, FLT_MAX, FLT_MAX};
        Vector3 cMax = {-FLT_MAX, -FLT_MAX, -FLT_MAX};

        for (size_t i = 0; i < triCount; ++i)
        {
            const auto& tri = ctx.AllTriangles[ctx.TriIndices[triStart + i]];
            nodeMin = Vector3Min(nodeMin, tri.min);
            nodeMax = Vector3Max(nodeMax, tri.max);
            cMin = Vector3Min(cMin, tri.center);
            cMax = Vector3Max(cMax, tri.center);
        }

        m_Nodes[nodeIdx].Min = nodeMin;
        m_Nodes[nodeIdx].Max = nodeMax;

        // Leaf condition
        if (triCount <= 4)
        {
            m_Nodes[nodeIdx].LeftOrFirst = (uint32_t)triStart;
            m_Nodes[nodeIdx].TriangleCount = (uint16_t)triCount;
            continue;
        }

        // Вибір осі (найдовша сторона боксу центроїдів)
        Vector3 extent = Vector3Subtract(cMax, cMin);
        int axis = 0;
        if (extent.y > extent.x && extent.y > extent.z) axis = 1;
        else if (extent.z > extent.x && extent.z > extent.y) axis = 2;

        float splitPos = 0;
        if (axis == 0) splitPos = cMin.x + extent.x * 0.5f;
        else if (axis == 1) splitPos = cMin.y + extent.y * 0.5f;
        else splitPos = cMin.z + extent.z * 0.5f;

        // 2. Використовуємо std::partition для безпечного розділення
        auto startIt = ctx.TriIndices.begin() + triStart;
        auto endIt = startIt + triCount;
        
        auto midIt = std::partition(startIt, endIt, [&](uint32_t idx) {
            const auto& tri = ctx.AllTriangles[idx];
            float val = (axis == 0) ? tri.center.x : (axis == 1) ? tri.center.y : tri.center.z;
            return val < splitPos;
        });

        size_t leftCount = std::distance(startIt, midIt);

        // Fallback: якщо розділення не спрацювало (всі центроїди в одній точці)
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

        // Створюємо дочірні вузли
        uint32_t leftIdx = (uint32_t)m_Nodes.size();
        m_Nodes.emplace_back();
        m_Nodes.emplace_back();

        // Оновлюємо поточний вузол
        m_Nodes[nodeIdx].LeftOrFirst = leftIdx;
        m_Nodes[nodeIdx].TriangleCount = 0;
        m_Nodes[nodeIdx].Axis = (uint16_t)axis;

        // Стек: спочатку правий, потім лівий (щоб лівий обробився першим)
        stack.push_back({leftIdx + 1, triStart + leftCount, triCount - leftCount});
        stack.push_back({leftIdx, triStart, leftCount});
    }
}

bool BVH::Raycast(const Ray& ray, float& t, Vector3& normal, int& meshIndex) const
{
    if (m_Nodes.empty())
    {
        return false;
    }

    uint32_t stack[64];
    uint32_t stackPtr = 0;
    stack[stackPtr++] = 0;

    bool hit = false;
    while (stackPtr > 0)
    {
        const BVHNode& node = m_Nodes[stack[--stackPtr]];

        RayCollision boxHit = GetRayCollisionBox(ray, {node.Min, node.Max});
        if (!boxHit.hit || boxHit.distance >= t)
        {
            continue;
        }

        if (node.IsLeaf())
        {
            for (uint32_t i = 0; i < node.TriangleCount; ++i)
            {
                const auto& tri = m_Triangles[node.LeftOrFirst + i];
                float triT = t;
                Vector3 triNormal;
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

            RayCollision leftHit = GetRayCollisionBox(ray, {m_Nodes[left].Min, m_Nodes[left].Max});
            RayCollision rightHit = GetRayCollisionBox(ray, {m_Nodes[right].Min, m_Nodes[right].Max});

            bool leftValid = leftHit.hit && leftHit.distance < t;
            bool rightValid = rightHit.hit && rightHit.distance < t;

            if (leftValid && rightValid)
            {
                if (leftHit.distance < rightHit.distance)
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
            else if (leftValid)
            {
                stack[stackPtr++] = left;
            }
            else if (rightValid)
            {
                stack[stackPtr++] = right;
            }
        }
    }
    return hit;
}

bool BVH::TestAxis(const Vector3& axis, const Vector3& v0, const Vector3& v1, const Vector3& v2,
                   const Vector3& boxCenter, const Vector3& boxHalfSize)
{
    float p0 = Vector3DotProduct(v0, axis);
    float p1 = Vector3DotProduct(v1, axis);
    float p2 = Vector3DotProduct(v2, axis);

    float r = boxHalfSize.x * fabsf(axis.x) +
              boxHalfSize.y * fabsf(axis.y) +
              boxHalfSize.z * fabsf(axis.z);

    float triMin = fminf(fminf(p0, p1), p2);
    float triMax = fmaxf(fmaxf(p0, p1), p2);

    float boxProj = Vector3DotProduct(boxCenter, axis);
    float boxMin = boxProj - r;
    float boxMax = boxProj + r;

    return !(triMin > boxMax || triMax < boxMin);
}

bool BVH::TriangleIntersectAABB(const CollisionTriangle& tri, const BoundingBox& box)
{
    Vector3 boxCenter = Vector3Scale(Vector3Add(box.min, box.max), 0.5f);
    Vector3 boxHalfSize = Vector3Scale(Vector3Subtract(box.max, box.min), 0.5f);

    Vector3 v0 = Vector3Subtract(tri.v0, boxCenter);
    Vector3 v1 = Vector3Subtract(tri.v1, boxCenter);
    Vector3 v2 = Vector3Subtract(tri.v2, boxCenter);

    Vector3 e0 = Vector3Subtract(v1, v0);
    Vector3 e1 = Vector3Subtract(v2, v1);
    Vector3 e2 = Vector3Subtract(v0, v2);

    if (!BVH::TestAxis({1, 0, 0}, v0, v1, v2, {0, 0, 0}, boxHalfSize))
    {
        return false;
    }
    if (!BVH::TestAxis({0, 1, 0}, v0, v1, v2, {0, 0, 0}, boxHalfSize))
    {
        return false;
    }
    if (!BVH::TestAxis({0, 0, 1}, v0, v1, v2, {0, 0, 0}, boxHalfSize))
    {
        return false;
    }

    Vector3 normal = Vector3CrossProduct(e0, e1);
    if (!BVH::TestAxis(normal, v0, v1, v2, {0, 0, 0}, boxHalfSize))
    {
        return false;
    }

    Vector3 axes[9] = {
        Vector3CrossProduct({1, 0, 0}, e0), Vector3CrossProduct({1, 0, 0}, e1), Vector3CrossProduct({1, 0, 0}, e2),
        Vector3CrossProduct({0, 1, 0}, e0), Vector3CrossProduct({0, 1, 0}, e1), Vector3CrossProduct({0, 1, 0}, e2),
        Vector3CrossProduct({0, 0, 1}, e0), Vector3CrossProduct({0, 0, 1}, e1), Vector3CrossProduct({0, 0, 1}, e2)};

    for (int i = 0; i < 9; i++)
    {
        if (Vector3Length(axes[i]) < 0.0001f)
        {
            continue;
        }
        if (!BVH::TestAxis(axes[i], v0, v1, v2, {0, 0, 0}, boxHalfSize))
        {
            return false;
        }
    }

    return true;
}

bool BVH::IntersectAABB(const BoundingBox& box, Vector3& outNormal, float& outDepth) const
{
    if (m_Nodes.empty())
    {
        return false;
    }

    uint32_t stack[64];
    uint32_t stackPtr = 0;
    stack[stackPtr++] = 0;

    bool hit = false;
    while (stackPtr > 0)
    {
        const BVHNode& node = m_Nodes[stack[--stackPtr]];

        if (!(node.Min.x <= box.max.x && node.Max.x >= box.min.x && node.Min.y <= box.max.y &&
              node.Max.y >= box.min.y && node.Min.z <= box.max.z && node.Max.z >= box.min.z))
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
                    Vector3 triNormal = tri.normal;

                    Vector3 boxCenter = Vector3Scale(Vector3Add(box.min, box.max), 0.5f);
                    float dist = Vector3DotProduct(Vector3Subtract(tri.v0, boxCenter), triNormal);
                    float radius = 0.5f * (fabsf(triNormal.x * (box.max.x - box.min.x)) +
                                           fabsf(triNormal.y * (box.max.y - box.min.y)) +
                                           fabsf(triNormal.z * (box.max.z - box.min.z)));

                    float depth = radius - fabsf(dist);
                    if (depth > outDepth)
                    {
                        outDepth = depth;
                        outNormal = (dist > 0) ? Vector3Scale(triNormal, -1.0f) : triNormal;
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
    if (m_Nodes.empty())
    {
        return;
    }

    uint32_t stack[64];
    uint32_t stackPtr = 0;
    stack[stackPtr++] = 0;

    while (stackPtr > 0)
    {
        const BVHNode& node = m_Nodes[stack[--stackPtr]];

        // AABB-AABB check
        if (!(node.Min.x <= box.max.x && node.Max.x >= box.min.x && node.Min.y <= box.max.y &&
              node.Max.y >= box.min.y && node.Min.z <= box.max.z && node.Max.z >= box.min.z))
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