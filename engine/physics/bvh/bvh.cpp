#include "bvh.h"
#include <algorithm>
#include <cfloat>
#include <future>
#include <raymath.h>

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

Ref<BVHNode> BVHBuilder::Build(const Model &model, const Matrix &transform)
{
    std::vector<CollisionTriangle> tris;

    for (int i = 0; i < model.meshCount; i++)
    {
        Mesh &mesh = model.meshes[i];
        Matrix meshTransform = MatrixMultiply(model.transform, transform);

        // This is a simplification. Usually meshes have their own transform in submodels.
        // For GLB/Raylib it might be in model.meshes[i].animVertices etc, but typically
        // we just use the indices.

        if (mesh.indices != nullptr)
        {
            for (int k = 0; k < mesh.triangleCount * 3; k += 3)
            {
                // Raylib's mesh.indices is unsigned short*, but if the mesh was 32-bit,
                // it might have been truncated. We can't fix the truncation here,
                // but we can ensure we read it consistently.
                int idx0 = mesh.indices[k];
                int idx1 = mesh.indices[k + 1];
                int idx2 = mesh.indices[k + 2];

                if (idx0 >= mesh.vertexCount || idx1 >= mesh.vertexCount ||
                    idx2 >= mesh.vertexCount)
                    continue;

                Vector3 v0 = {mesh.vertices[idx0 * 3], mesh.vertices[idx0 * 3 + 1],
                              mesh.vertices[idx0 * 3 + 2]};
                Vector3 v1 = {mesh.vertices[idx1 * 3], mesh.vertices[idx1 * 3 + 1],
                              mesh.vertices[idx1 * 3 + 2]};
                Vector3 v2 = {mesh.vertices[idx2 * 3], mesh.vertices[idx2 * 3 + 1],
                              mesh.vertices[idx2 * 3 + 2]};

                tris.emplace_back(Vector3Transform(v0, meshTransform),
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

                tris.emplace_back(Vector3Transform(v0, meshTransform),
                                  Vector3Transform(v1, meshTransform),
                                  Vector3Transform(v2, meshTransform), i);
            }
        }
    }

    if (tris.empty())
        return nullptr;

    return BuildRecursive(tris, 0, tris.size(), 0);
}

Ref<BVHNode> BVHBuilder::BuildRecursive(std::vector<CollisionTriangle> &tris, size_t start,
                                        size_t end, int depth)
{
    auto node = CreateRef<BVHNode>();

    // Calculate Bounds
    node->min = {1e30f, 1e30f, 1e30f};
    node->max = {-1e30f, -1e30f, -1e30f};
    for (size_t i = start; i < end; ++i)
    {
        node->min = Vector3Min(node->min, tris[i].min);
        node->max = Vector3Max(node->max, tris[i].max);
    }

    size_t count = end - start;
    if (count <= 4 || depth > 20)
    {
        node->triangles.reserve(count);
        for (size_t i = start; i < end; ++i)
            node->triangles.push_back(std::move(tris[i]));
        return node;
    }

    // Determine split axis based on largest dimension
    Vector3 size = Vector3Subtract(node->max, node->min);
    int axis = 0;
    if (size.y > size.x && size.y > size.z)
        axis = 1;
    else if (size.z > size.x && size.z > size.y)
        axis = 2;

    // Partition in-place using median element (nth_element is O(N))
    size_t mid = start + count / 2;
    std::nth_element(tris.begin() + start, tris.begin() + mid, tris.begin() + end,
                     [axis](const CollisionTriangle &a, const CollisionTriangle &b)
                     {
                         if (axis == 0)
                             return a.center.x < b.center.x;
                         if (axis == 1)
                             return a.center.y < b.center.y;
                         return a.center.z < b.center.z;
                     });

    node->left = BuildRecursive(tris, start, mid, depth + 1);
    node->right = BuildRecursive(tris, mid, end, depth + 1);

    return node;
}

bool BVHBuilder::Raycast(const BVHNode *node, const Ray &ray, float &t, Vector3 &normal,
                         int &meshIndex)
{
    if (!node)
        return false;
    return RayInternal(node, ray, t, normal, meshIndex);
}

static bool IntersectRayAABB(const Ray &ray, Vector3 min, Vector3 max, float &t)
{
    float t1 = (min.x - ray.position.x) / ray.direction.x;
    float t2 = (max.x - ray.position.x) / ray.direction.x;
    float t3 = (min.y - ray.position.y) / ray.direction.y;
    float t4 = (max.y - ray.position.y) / ray.direction.y;
    float t5 = (min.z - ray.position.z) / ray.direction.z;
    float t6 = (max.z - ray.position.z) / ray.direction.z;

    float tmin = fmaxf(fmaxf(fminf(t1, t2), fminf(t3, t4)), fminf(t5, t6));
    float tmax = fminf(fminf(fmaxf(t1, t2), fmaxf(t3, t4)), fmaxf(t5, t6));

    if (tmax < 0 || tmin > tmax)
        return false;
    t = fmaxf(0.0f, tmin);
    return true;
}

bool BVHBuilder::RayInternal(const BVHNode *node, const Ray &ray, float &t, Vector3 &normal,
                             int &meshIndex)
{
    float tBox;
    if (!IntersectRayAABB(ray, node->min, node->max, tBox))
        return false;

    if (tBox > t)
        return false;

    bool hit = false;
    if (node->IsLeaf())
    {
        for (const auto &tri : node->triangles)
        {
            float triT;
            if (tri.IntersectsRay(ray, triT))
            {
                if (triT < t)
                {
                    t = triT;
                    normal = Vector3Normalize(Vector3CrossProduct(Vector3Subtract(tri.v1, tri.v0),
                                                                  Vector3Subtract(tri.v2, tri.v0)));
                    meshIndex = tri.meshIndex;
                    hit = true;
                }
            }
        }
    }
    else
    {
        if (RayInternal(node->left.get(), ray, t, normal, meshIndex))
            hit = true;
        if (RayInternal(node->right.get(), ray, t, normal, meshIndex))
            hit = true;
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

    // Box normals
    if (!TestAxis({1, 0, 0}, v0, v1, v2, {0, 0, 0}, boxHalfSize))
        return false;
    if (!TestAxis({0, 1, 0}, v0, v1, v2, {0, 0, 0}, boxHalfSize))
        return false;
    if (!TestAxis({0, 0, 1}, v0, v1, v2, {0, 0, 0}, boxHalfSize))
        return false;

    // Triangle normal
    Vector3 normal = Vector3CrossProduct(e0, e1);
    if (!TestAxis(normal, v0, v1, v2, {0, 0, 0}, boxHalfSize))
        return false;

    // 9 edges cross products
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

static bool IntersectAABBInternal(const BVHNode *node, const BoundingBox &box, Vector3 &outNormal,
                                  float &outDepth)
{
    if (!(node->min.x <= box.max.x && node->max.x >= box.min.x && node->min.y <= box.max.y &&
          node->max.y >= box.min.y && node->min.z <= box.max.z && node->max.z >= box.min.z))
        return false;

    bool hit = false;
    if (node->IsLeaf())
    {
        for (const auto &tri : node->triangles)
        {
            if (TriangleIntersectAABB(tri, box))
            {
                // For resolution, we take the triangle normal and the penetration depth
                Vector3 normal = Vector3Normalize(Vector3CrossProduct(
                    Vector3Subtract(tri.v1, tri.v0), Vector3Subtract(tri.v2, tri.v0)));

                // Estimate penetration depth along normal
                // This is a simplification. A better way would be actual SAT penetration.
                Vector3 boxCenter = Vector3Scale(Vector3Add(box.min, box.max), 0.5f);
                float dist = Vector3DotProduct(Vector3Subtract(tri.v0, boxCenter), normal);
                float radius = 0.5f * (fabsf(normal.x * (box.max.x - box.min.x)) +
                                       fabsf(normal.y * (box.max.y - box.min.y)) +
                                       fabsf(normal.z * (box.max.z - box.min.z)));

                float depth = radius - fabsf(dist);

                if (depth > outDepth)
                {
                    outDepth = depth;
                    outNormal = (dist > 0) ? Vector3Scale(normal, -1.0f) : normal;
                    hit = true;
                }
            }
        }
    }
    else
    {
        if (IntersectAABBInternal(node->left.get(), box, outNormal, outDepth))
            hit = true;
        if (IntersectAABBInternal(node->right.get(), box, outNormal, outDepth))
            hit = true;
    }
    return hit;
}

bool BVHBuilder::IntersectAABB(const BVHNode *node, const BoundingBox &box, Vector3 &outNormal,
                               float &outDepth)
{
    if (!node)
        return false;
    outDepth = -1.0f;
    return IntersectAABBInternal(node, box, outNormal, outDepth);
}

std::future<Ref<BVHNode>> BVHBuilder::BuildAsync(const Model &model, const Matrix &transform)
{
    auto task = std::make_shared<std::packaged_task<Ref<BVHNode>()>>(
        [model, transform]() { return Build(model, transform); });

    std::future<Ref<BVHNode>> future = task->get_future();
    std::async(std::launch::async, [task]() { (*task)(); });
    return future;
}

} // namespace CHEngine
