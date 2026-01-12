#include "bvh.h"
#include "engine/core/thread_dispatcher.h"
#include <algorithm>
#include <cfloat>
#include <raymath.h>

namespace CHEngine
{
CollisionTriangle::CollisionTriangle(const Vector3 &a, const Vector3 &b, const Vector3 &c)
    : v0(a), v1(b), v2(c)
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
                unsigned short idx0 = mesh.indices[k];
                unsigned short idx1 = mesh.indices[k + 1];
                unsigned short idx2 = mesh.indices[k + 2];

                Vector3 v0 = {mesh.vertices[idx0 * 3], mesh.vertices[idx0 * 3 + 1],
                              mesh.vertices[idx0 * 3 + 2]};
                Vector3 v1 = {mesh.vertices[idx1 * 3], mesh.vertices[idx1 * 3 + 1],
                              mesh.vertices[idx1 * 3 + 2]};
                Vector3 v2 = {mesh.vertices[idx2 * 3], mesh.vertices[idx2 * 3 + 1],
                              mesh.vertices[idx2 * 3 + 2]};

                tris.emplace_back(Vector3Transform(v0, meshTransform),
                                  Vector3Transform(v1, meshTransform),
                                  Vector3Transform(v2, meshTransform));
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
                                  Vector3Transform(v2, meshTransform));
            }
        }
    }

    if (tris.empty())
        return nullptr;

    return BuildRecursive(tris, 0);
}

Ref<BVHNode> BVHBuilder::BuildRecursive(std::vector<CollisionTriangle> &tris, int depth)
{
    auto node = CreateRef<BVHNode>();

    // Calculate Bounds
    node->min = {1e30f, 1e30f, 1e30f};
    node->max = {-1e30f, -1e30f, -1e30f};
    for (const auto &tri : tris)
    {
        node->min = Vector3Min(node->min, tri.min);
        node->max = Vector3Max(node->max, tri.max);
    }

    if (tris.size() <= 4 || depth > 20)
    {
        node->triangles = std::move(tris);
        return node;
    }

    // For small sets, use fast median split instead of SAH
    if (tris.size() <= 16)
    {
        // Fast median split
        Vector3 size = Vector3Subtract(node->max, node->min);
        int axis = 0;
        if (size.y > size.x && size.y > size.z)
            axis = 1;
        if (size.z > size.x && size.z > size.y)
            axis = 2;

        std::sort(tris.begin(), tris.end(),
                  [axis](const CollisionTriangle &a, const CollisionTriangle &b)
                  {
                      if (axis == 0)
                          return a.center.x < b.center.x;
                      if (axis == 1)
                          return a.center.y < b.center.y;
                      return a.center.z < b.center.z;
                  });

        size_t mid = tris.size() / 2;
        std::vector<CollisionTriangle> leftTris(tris.begin(), tris.begin() + mid);
        std::vector<CollisionTriangle> rightTris(tris.begin() + mid, tris.end());

        node->left = BuildRecursive(leftTris, depth + 1);
        node->right = BuildRecursive(rightTris, depth + 1);
        return node;
    }

    // SAH Split evaluation - O(N) using prefix/suffix bounds
    int bestAxis = -1;
    float bestCost = FLT_MAX;
    size_t bestMid = 0;

    Vector3 size = Vector3Subtract(node->max, node->min);
    int primaryAxis = (size.y > size.x && size.y > size.z) ? 1 : (size.z > size.x ? 2 : 0);

    std::sort(tris.begin(), tris.end(),
              [primaryAxis](const CollisionTriangle &a, const CollisionTriangle &b)
              {
                  if (primaryAxis == 0)
                      return a.center.x < b.center.x;
                  if (primaryAxis == 1)
                      return a.center.y < b.center.y;
                  return a.center.z < b.center.z;
              });

    size_t n = tris.size();
    std::vector<BoundingBox> rightBounds(n);
    BoundingBox currentRight = {{1e30f, 1e30f, 1e30f}, {-1e30f, -1e30f, -1e30f}};

    for (int i = (int)n - 1; i >= 0; i--)
    {
        currentRight.min = Vector3Min(currentRight.min, tris[i].min);
        currentRight.max = Vector3Max(currentRight.max, tris[i].max);
        rightBounds[i] = currentRight;
    }

    BoundingBox leftBound = {{1e30f, 1e30f, 1e30f}, {-1e30f, -1e30f, -1e30f}};
    for (size_t i = 1; i < n; i++)
    {
        leftBound.min = Vector3Min(leftBound.min, tris[i - 1].min);
        leftBound.max = Vector3Max(leftBound.max, tris[i - 1].max);

        Vector3 sL = Vector3Subtract(leftBound.max, leftBound.min);
        float saL = 2.0f * (sL.x * sL.y + sL.y * sL.z + sL.z * sL.x);

        Vector3 sR = Vector3Subtract(rightBounds[i].max, rightBounds[i].min);
        float saR = 2.0f * (sR.x * sR.y + sR.y * sR.z + sR.z * sR.x);

        float cost = saL * (float)i + saR * (float)(n - i);
        if (cost < bestCost)
        {
            bestCost = cost;
            bestAxis = primaryAxis;
            bestMid = i;
        }
    }

    // Already sorted by primaryAxis, just split
    std::vector<CollisionTriangle> leftTris(tris.begin(), tris.begin() + bestMid);
    std::vector<CollisionTriangle> rightTris(tris.begin() + bestMid, tris.end());

    node->left = BuildRecursive(leftTris, depth + 1);
    node->right = BuildRecursive(rightTris, depth + 1);

    return node;
}

bool BVHBuilder::Raycast(const BVHNode *node, const Ray &ray, float &t, Vector3 &normal)
{
    if (!node)
        return false;
    return RayInternal(node, ray, t, normal);
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

bool BVHBuilder::RayInternal(const BVHNode *node, const Ray &ray, float &t, Vector3 &normal)
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
                    hit = true;
                }
            }
        }
    }
    else
    {
        if (RayInternal(node->left.get(), ray, t, normal))
            hit = true;
        if (RayInternal(node->right.get(), ray, t, normal))
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
    return ThreadDispatcher::DispatchAsync([model, transform]() -> Ref<BVHNode>
                                           { return Build(model, transform); });
}

} // namespace CHEngine
