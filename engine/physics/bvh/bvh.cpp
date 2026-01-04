#include "bvh.h"
#include <algorithm>
#include <raymath.h>

namespace CH
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

Scope<BVHNode> BVHBuilder::Build(const Model &model, const Matrix &transform)
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

Scope<BVHNode> BVHBuilder::BuildRecursive(std::vector<CollisionTriangle> &tris, int depth)
{
    auto node = CreateScope<BVHNode>();

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

    // Split
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

bool BVHBuilder::Raycast(const BVHNode *node, const Ray &ray, float &t, Vector3 &normal)
{
    if (!node)
        return false;
    return RayInternal(node, ray, t, normal);
}

static bool IntersectAABB(const Ray &ray, Vector3 min, Vector3 max, float &t)
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
    t = tmin;
    return true;
}

bool BVHBuilder::RayInternal(const BVHNode *node, const Ray &ray, float &t, Vector3 &normal)
{
    float tBox;
    if (!IntersectAABB(ray, node->min, node->max, tBox))
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
} // namespace CH
