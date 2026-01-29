#include "bvh.h"
#include "algorithm"
#include "cfloat"
#include "future"
#include "raymath.h"

namespace CHEngine
{
CollisionTriangle::CollisionTriangle(const Vector3 &a, const Vector3 &b, const Vector3 &c,
                                     int index)
    : v0(a), v1(b), v2(c), meshIndex(index)
{
    min = Vector3Min(Vector3Min(v0, v1), v2);
    max = Vector3Max(Vector3Max(v0, v1), v2);
    center = Vector3Scale(Vector3Add(Vector3Add(v0, v1), v2), 1.0f / 3.0f);
}

bool CollisionTriangle::IntersectsRay(const Ray &ray, float &t) const
{
    Vector3 edge1 = Vector3Subtract(v1, v0);
    Vector3 edge2 = Vector3Subtract(v2, v0);
    Vector3 pvec = Vector3CrossProduct(ray.direction, edge2);
    float det = Vector3DotProduct(edge1, pvec);

    if (det > -0.000001f && det < 0.000001f)
        return false;

    float invDet = 1.0f / det;
    Vector3 tvec = Vector3Subtract(ray.position, v0);
    float u = Vector3DotProduct(tvec, pvec) * invDet;

    if (u < 0.0f || u > 1.0f)
        return false;

    Vector3 qvec = Vector3CrossProduct(tvec, edge1);
    float v = Vector3DotProduct(ray.direction, qvec) * invDet;

    if (v < 0.0f || u + v > 1.0f)
        return false;

    t = Vector3DotProduct(edge2, qvec) * invDet;
    return t > 0.000001f;
}

std::shared_ptr<BVH> BVH::Build(const Model &model, const Matrix &transform)
{
    if (model.meshCount == 0)
        return nullptr;

    auto bvh = std::make_shared<BVH>();
    std::vector<CollisionTriangle> allTris;

    for (int i = 0; i < model.meshCount; i++)
    {
        Mesh &mesh = model.meshes[i];
        if (mesh.vertices == nullptr)
            continue;

        Matrix meshTransform = MatrixMultiply(model.transform, transform);

        if (mesh.indices != nullptr && mesh.triangleCount > 0)
        {
            for (int k = 0; k < mesh.triangleCount * 3; k += 3)
            {
                uint32_t idx0 = mesh.indices[k];
                uint32_t idx1 = mesh.indices[k + 1];
                uint32_t idx2 = mesh.indices[k + 2];

                Vector3 v0 = {mesh.vertices[idx0 * 3], mesh.vertices[idx0 * 3 + 1],
                              mesh.vertices[idx0 * 3 + 2]};
                Vector3 v1 = {mesh.vertices[idx1 * 3], mesh.vertices[idx1 * 3 + 1],
                              mesh.vertices[idx1 * 3 + 2]};
                Vector3 v2 = {mesh.vertices[idx2 * 3], mesh.vertices[idx2 * 3 + 1],
                              mesh.vertices[idx2 * 3 + 2]};

                allTris.emplace_back(Vector3Transform(v0, meshTransform),
                                     Vector3Transform(v1, meshTransform),
                                     Vector3Transform(v2, meshTransform), i);
            }
        }
        else
        {
            for (int k = 0; k < mesh.vertexCount * 3; k += 9)
            {
                Vector3 v0 = {mesh.vertices[k], mesh.vertices[k + 1], mesh.vertices[k + 2]};
                Vector3 v1 = {mesh.vertices[k + 3], mesh.vertices[k + 4], mesh.vertices[k + 5]};
                Vector3 v2 = {mesh.vertices[k + 6], mesh.vertices[k + 7], mesh.vertices[k + 8]};

                allTris.emplace_back(Vector3Transform(v0, meshTransform),
                                     Vector3Transform(v1, meshTransform),
                                     Vector3Transform(v2, meshTransform), i);
            }
        }
    }

    if (allTris.empty())
        return nullptr;

    bvh->m_Triangles = std::move(allTris);
    bvh->m_Nodes.reserve(bvh->m_Triangles.size() * 2);
    bvh->m_Nodes.emplace_back(); // Root

    BuildContext ctx(bvh->m_Triangles);
    bvh->BuildRecursive(ctx, 0, 0, bvh->m_Triangles.size(), 0);

    // Reorder triangles based on serial indices
    std::vector<CollisionTriangle> reorderedTris;
    reorderedTris.reserve(bvh->m_Triangles.size());
    for (uint32_t idx : ctx.TriIndices)
        reorderedTris.push_back(bvh->m_Triangles[idx]);
    bvh->m_Triangles = std::move(reorderedTris);

    return bvh;
}

void BVH::BuildRecursive(BuildContext &ctx, uint32_t nodeIdx, size_t triStart, size_t triCount,
                         int depth)
{
    BVHNode &node = m_Nodes[nodeIdx];

    // Calculate bounds
    node.Min = {FLT_MAX, FLT_MAX, FLT_MAX};
    node.Max = {-FLT_MAX, -FLT_MAX, -FLT_MAX};
    Vector3 centroidMin = {FLT_MAX, FLT_MAX, FLT_MAX};
    Vector3 centroidMax = {-FLT_MAX, -FLT_MAX, -FLT_MAX};

    for (size_t i = 0; i < triCount; ++i)
    {
        const auto &tri = ctx.AllTriangles[ctx.TriIndices[triStart + i]];
        node.Min = Vector3Min(node.Min, tri.min);
        node.Max = Vector3Max(node.Max, tri.max);
        centroidMin = Vector3Min(centroidMin, tri.center);
        centroidMax = Vector3Max(centroidMax, tri.center);
    }

    if (triCount <= 4 || depth > 32)
    {
        node.LeftOrFirst = (uint32_t)triStart;
        node.TriangleCount = (uint16_t)triCount;
        return;
    }

    // Split based on centroids
    Vector3 size = Vector3Subtract(centroidMax, centroidMin);
    int axis = 0;
    if (size.y > size.x && size.y > size.z)
        axis = 1;
    else if (size.z > size.x && size.z > size.y)
        axis = 2;

    float splitPos = centroidMin.x + size.x * 0.5f;
    if (axis == 1)
        splitPos = centroidMin.y + size.y * 0.5f;
    else if (axis == 2)
        splitPos = centroidMin.z + size.z * 0.5f;

    // Partition
    size_t i = triStart;
    size_t j = triStart + triCount - 1;
    while (i <= j)
    {
        float val = ctx.AllTriangles[ctx.TriIndices[i]].center.x;
        if (axis == 1)
            val = ctx.AllTriangles[ctx.TriIndices[i]].center.y;
        else if (axis == 2)
            val = ctx.AllTriangles[ctx.TriIndices[i]].center.z;

        if (val < splitPos)
            i++;
        else
        {
            std::swap(ctx.TriIndices[i], ctx.TriIndices[j]);
            if (j == 0)
                break;
            j--;
        }
    }

    size_t leftCount = i - triStart;
    if (leftCount == 0 || leftCount == triCount)
    {
        leftCount = triCount / 2;
        std::nth_element(ctx.TriIndices.begin() + triStart,
                         ctx.TriIndices.begin() + triStart + leftCount,
                         ctx.TriIndices.begin() + triStart + triCount,
                         [&](uint32_t a, uint32_t b)
                         {
                             const auto &triA = ctx.AllTriangles[a];
                             const auto &triB = ctx.AllTriangles[b];
                             if (axis == 0)
                                 return triA.center.x < triB.center.x;
                             if (axis == 1)
                                 return triA.center.y < triB.center.y;
                             return triA.center.z < triB.center.z;
                         });
    }

    uint32_t leftIdx = (uint32_t)m_Nodes.size();
    m_Nodes.emplace_back();
    m_Nodes.emplace_back();

    node.LeftOrFirst = leftIdx;
    node.TriangleCount = 0;
    node.Axis = (uint16_t)axis;

    BuildRecursive(ctx, leftIdx, triStart, leftCount, depth + 1);
    BuildRecursive(ctx, leftIdx + 1, triStart + leftCount, triCount - leftCount, depth + 1);
}

bool BVH::Raycast(const Ray &ray, float &t, Vector3 &normal, int &meshIndex) const
{
    if (m_Nodes.empty())
        return false;

    uint32_t stack[64];
    uint32_t stackPtr = 0;
    stack[stackPtr++] = 0;

    bool hit = false;
    while (stackPtr > 0)
    {
        const BVHNode &node = m_Nodes[stack[--stackPtr]];

        RayCollision boxHit = GetRayCollisionBox(ray, {node.Min, node.Max});
        if (!boxHit.hit || boxHit.distance >= t)
            continue;

        if (node.IsLeaf())
        {
            for (uint32_t i = 0; i < node.TriangleCount; ++i)
            {
                const auto &tri = m_Triangles[node.LeftOrFirst + i];
                RayCollision triHit = GetRayCollisionTriangle(ray, tri.v0, tri.v1, tri.v2);
                if (triHit.hit && triHit.distance < t)
                {
                    t = triHit.distance;
                    normal = triHit.normal;
                    meshIndex = tri.meshIndex;
                    hit = true;
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

static bool TestAxis(const Vector3 &axis, const Vector3 &v0, const Vector3 &v1, const Vector3 &v2,
                     const Vector3 &boxCenter, const Vector3 &boxHalfSize)
{
    float p0 = Vector3DotProduct(v0, axis);
    float p1 = Vector3DotProduct(v1, axis);
    float p2 = Vector3DotProduct(v2, axis);

    float r = boxHalfSize.x * fabsf(Vector3DotProduct({1, 0, 0}, axis)) +
              boxHalfSize.y * fabsf(Vector3DotProduct({0, 1, 0}, axis)) +
              boxHalfSize.z * fabsf(Vector3DotProduct({0, 0, 1}, axis));

    float triMin = fminf(fminf(p0, p1), p2);
    float triMax = fmaxf(fmaxf(p0, p1), p2);

    float boxProj = Vector3DotProduct(boxCenter, axis);
    float boxMin = boxProj - r;
    float boxMax = boxProj + r;

    return !(triMin > boxMax || triMax < boxMin);
}

static bool TriangleIntersectAABB(const CollisionTriangle &tri, const BoundingBox &box)
{
    Vector3 boxCenter = Vector3Scale(Vector3Add(box.min, box.max), 0.5f);
    Vector3 boxHalfSize = Vector3Scale(Vector3Subtract(box.max, box.min), 0.5f);

    Vector3 v0 = Vector3Subtract(tri.v0, boxCenter);
    Vector3 v1 = Vector3Subtract(tri.v1, boxCenter);
    Vector3 v2 = Vector3Subtract(tri.v2, boxCenter);

    Vector3 e0 = Vector3Subtract(v1, v0);
    Vector3 e1 = Vector3Subtract(v2, v1);
    Vector3 e2 = Vector3Subtract(v0, v2);

    if (!TestAxis({1, 0, 0}, v0, v1, v2, {0, 0, 0}, boxHalfSize))
        return false;
    if (!TestAxis({0, 1, 0}, v0, v1, v2, {0, 0, 0}, boxHalfSize))
        return false;
    if (!TestAxis({0, 0, 1}, v0, v1, v2, {0, 0, 0}, boxHalfSize))
        return false;

    Vector3 normal = Vector3CrossProduct(e0, e1);
    if (!TestAxis(normal, v0, v1, v2, {0, 0, 0}, boxHalfSize))
        return false;

    Vector3 axes[9] = {Vector3CrossProduct({1, 0, 0}, e0), Vector3CrossProduct({1, 0, 0}, e1),
                       Vector3CrossProduct({1, 0, 0}, e2), Vector3CrossProduct({0, 1, 0}, e0),
                       Vector3CrossProduct({0, 1, 0}, e1), Vector3CrossProduct({0, 1, 0}, e2),
                       Vector3CrossProduct({0, 0, 1}, e0), Vector3CrossProduct({0, 0, 1}, e1),
                       Vector3CrossProduct({0, 0, 1}, e2)};

    for (int i = 0; i < 9; i++)
    {
        if (Vector3Length(axes[i]) < 0.0001f)
            continue;
        if (!TestAxis(axes[i], v0, v1, v2, {0, 0, 0}, boxHalfSize))
            return false;
    }

    return true;
}

bool BVH::IntersectAABB(const BoundingBox &box, Vector3 &outNormal, float &outDepth) const
{
    if (m_Nodes.empty())
        return false;

    uint32_t stack[64];
    uint32_t stackPtr = 0;
    stack[stackPtr++] = 0;

    bool hit = false;
    while (stackPtr > 0)
    {
        const BVHNode &node = m_Nodes[stack[--stackPtr]];

        if (!(node.Min.x <= box.max.x && node.Max.x >= box.min.x && node.Min.y <= box.max.y &&
              node.Max.y >= box.min.y && node.Min.z <= box.max.z && node.Max.z >= box.min.z))
            continue;

        if (node.IsLeaf())
        {
            for (uint32_t i = 0; i < node.TriangleCount; ++i)
            {
                const auto &tri = m_Triangles[node.LeftOrFirst + i];
                if (TriangleIntersectAABB(tri, box))
                {
                    Vector3 triNormal = Vector3Normalize(Vector3CrossProduct(
                        Vector3Subtract(tri.v1, tri.v0), Vector3Subtract(tri.v2, tri.v0)));

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

std::future<std::shared_ptr<BVH>> BVH::BuildAsync(const Model &model, const Matrix &transform)
{
    return std::async(std::launch::async, [model, transform]() { return Build(model, transform); });
}
} // namespace CHEngine
