//
// Created by I#Oleg
//
#include <Collision/CollisionSystem.h>
#include <algorithm>
#include <functional>
#include <limits>
#include <raylib.h>

// ================== CollisionTriangle Implementation ==================

CollisionTriangle::CollisionTriangle(const Vector3 &a, const Vector3 &b, const Vector3 &c)
    : v0(a), v1(b), v2(c)
{
    // Calculate normal using cross product
    Vector3 edge1 = Vector3Subtract(v1, v0);
    Vector3 edge2 = Vector3Subtract(v2, v0);
    normal = Vector3Normalize(Vector3CrossProduct(edge1, edge2));
}

bool CollisionTriangle::Intersects(const CollisionRay &ray, float &t) const
{
    // Möller-Trumbore ray-triangle intersection algorithm

    Vector3 edge1 = Vector3Subtract(v1, v0);
    Vector3 edge2 = Vector3Subtract(v2, v0);
    Vector3 h = Vector3CrossProduct(ray.direction, edge2);
    float a = Vector3DotProduct(edge1, h);

    if (a > -EPSILON && a < EPSILON)
        return false; // Ray is parallel to triangle

    float f = 1.0f / a;
    Vector3 s = Vector3Subtract(ray.origin, v0);
    float u = f * Vector3DotProduct(s, h);

    if (u < 0.0f || u > 1.0f)
        return false;

    Vector3 q = Vector3CrossProduct(s, edge1);
    float v = f * Vector3DotProduct(ray.direction, q);

    if (v < 0.0f || u + v > 1.0f)
        return false;

    t = f * Vector3DotProduct(edge2, q);
    return t > EPSILON; // Ray intersection
}

Vector3 CollisionTriangle::GetCenter() const
{
    return Vector3Scale(Vector3Add(Vector3Add(v0, v1), v2), 1.0f / 3.0f);
}

Vector3 CollisionTriangle::GetMin() const
{
    return {fminf(fminf(v0.x, v1.x), v2.x), fminf(fminf(v0.y, v1.y), v2.y),
            fminf(fminf(v0.z, v1.z), v2.z)};
}

Vector3 CollisionTriangle::GetMax() const
{
    return {fmaxf(fmaxf(v0.x, v1.x), v2.x), fmaxf(fmaxf(v0.y, v1.y), v2.y),
            fmaxf(fmaxf(v0.z, v1.z), v2.z)};
}

// ================== CollisionRay Implementation ==================

CollisionRay::CollisionRay(const Vector3 &orig, const Vector3 &dir)
    : origin(orig), direction(Vector3Normalize(dir))
{
}

// ================== BVHNode Implementation ==================

BVHNode::BVHNode() : isLeaf(false)
{
    minBounds = {FLT_MAX, FLT_MAX, FLT_MAX};
    maxBounds = {-FLT_MAX, -FLT_MAX, -FLT_MAX};
}

bool BVHNode::Intersects(const Vector3 &min, const Vector3 &max) const
{
    return (minBounds.x <= max.x && maxBounds.x >= min.x) &&
           (minBounds.y <= max.y && maxBounds.y >= min.y) &&
           (minBounds.z <= max.z && maxBounds.z >= min.z);
}

bool BVHNode::IntersectsRay(const CollisionRay &ray, float &t) const
{
    // Ray-box intersection using slab method
    Vector3 invDir = {1.0f / ray.direction.x, 1.0f / ray.direction.y, 1.0f / ray.direction.z};

    float t1 = (minBounds.x - ray.origin.x) * invDir.x;
    float t2 = (maxBounds.x - ray.origin.x) * invDir.x;
    if (t1 > t2)
    {
        float temp = t1;
        t1 = t2;
        t2 = temp;
    }

    float t3 = (minBounds.y - ray.origin.y) * invDir.y;
    float t4 = (maxBounds.y - ray.origin.y) * invDir.y;
    if (t3 > t4)
    {
        float temp = t3;
        t3 = t4;
        t4 = temp;
    }

    float t5 = (minBounds.z - ray.origin.z) * invDir.z;
    float t6 = (maxBounds.z - ray.origin.z) * invDir.z;
    if (t5 > t6)
    {
        float temp = t5;
        t5 = t6;
        t6 = temp;
    }

    float tmin = fmaxf(fmaxf(t1, t3), t5);
    float tmax = fminf(fminf(t2, t4), t6);

    if (tmax < 0 || tmin > tmax)
        return false;

    t = tmin > 0 ? tmin : tmax;
    return t >= 0;
}

// ================== Collision Implementation ==================

Collision::Collision(const Vector3 &center, const Vector3 &size) { Update(center, size); }

// Copy constructor
Collision::Collision(const Collision &other)
    : m_min(other.m_min), m_max(other.m_max), m_mesh(other.m_mesh), m_triangles(other.m_triangles),
      m_useBVH(other.m_useBVH)
{
    // Deep copy of BVH tree if it exists
    if (other.m_bvhRoot)
    {
        // For now, rebuild BVH instead of deep copying the tree structure
        // This is simpler and safer than copying the entire tree
        m_useBVH = false; // Will need to rebuild BVH manually if needed
    }
}

// Copy assignment operator
Collision &Collision::operator=(const Collision &other)
{
    if (this != &other)
    {
        m_min = other.m_min;
        m_max = other.m_max;
        m_mesh = other.m_mesh;
        m_triangles = other.m_triangles;
        m_useBVH = other.m_useBVH;

        // Reset BVH - will need to be rebuilt if needed
        m_bvhRoot.reset();
        if (other.m_bvhRoot)
        {
            m_useBVH = false; // Will need to rebuild BVH manually if needed
        }
    }
    return *this;
}

void Collision::Update(const Vector3 &center, const Vector3 &size)
{
    m_min = Vector3Subtract(center, Vector3Scale(size, 0.5f));
    m_max = Vector3Add(center, Vector3Scale(size, 0.5f));
}

bool Collision::Intersects(const Collision &other) const
{
    return (m_min.x <= other.m_max.x && m_max.x >= other.m_min.x) &&
           (m_min.y <= other.m_max.y && m_max.y >= other.m_min.y) &&
           (m_min.z <= other.m_max.z && m_max.z >= other.m_min.z);
}

bool Collision::Contains(const Vector3 &point) const
{
    return (point.x >= m_min.x && point.x <= m_max.x) &&
           (point.y >= m_min.y && point.y <= m_max.y) && (point.z >= m_min.z && point.z <= m_max.z);
}

Vector3 Collision::GetMin() const { return m_min; }
Vector3 Collision::GetMax() const { return m_max; }

void Collision::CalculateFromModel(Model *model)
{
    if (!model || model->meshCount == 0)
    {
        TraceLog(LOG_WARNING, "Invalid model provided for collision calculation");
        return;
    }

    Vector3 min = {FLT_MAX, FLT_MAX, FLT_MAX};
    Vector3 max = {-FLT_MAX, -FLT_MAX, -FLT_MAX};

    for (int m = 0; m < model->meshCount; m++)
    {
        Mesh &mesh = model->meshes[m];
        for (int i = 0; i < mesh.vertexCount; i++)
        {
            Vector3 v = {mesh.vertices[i * 3 + 0], mesh.vertices[i * 3 + 1],
                         mesh.vertices[i * 3 + 2]};

            min.x = fminf(min.x, v.x);
            min.y = fminf(min.y, v.y);
            min.z = fminf(min.z, v.z);

            max.x = fmaxf(max.x, v.x);
            max.y = fmaxf(max.y, v.y);
            max.z = fmaxf(max.z, v.z);
        }
    }

    Vector3 size = Vector3Subtract(max, min);
    Vector3 center = Vector3Add(min, Vector3Scale(size, 0.5f));

    Update(center, size);

    TraceLog(LOG_INFO, "Model collision calculated: center=(%.2f,%.2f,%.2f) size=(%.2f,%.2f,%.2f)",
             center.x, center.y, center.z, size.x, size.y, size.z);
}

void Collision::CalculateFromModel(Model *model, const Matrix &transform)
{
    if (!model || model->meshCount == 0)
    {
        TraceLog(LOG_WARNING, "Invalid model provided for transformed collision calculation");
        return;
    }

    Vector3 min = {FLT_MAX, FLT_MAX, FLT_MAX};
    Vector3 max = {-FLT_MAX, -FLT_MAX, -FLT_MAX};

    for (int m = 0; m < model->meshCount; m++)
    {
        Mesh &mesh = model->meshes[m];
        for (int i = 0; i < mesh.vertexCount; i++)
        {
            Vector3 v = {mesh.vertices[i * 3 + 0], mesh.vertices[i * 3 + 1],
                         mesh.vertices[i * 3 + 2]};

            // Apply transform matrix to vertex
            Vector3 transformedV = Vector3Transform(v, transform);

            min.x = fminf(min.x, transformedV.x);
            min.y = fminf(min.y, transformedV.y);
            min.z = fminf(min.z, transformedV.z);

            max.x = fmaxf(max.x, transformedV.x);
            max.y = fmaxf(max.y, transformedV.y);
            max.z = fmaxf(max.z, transformedV.z);
        }
    }

    Vector3 size = Vector3Subtract(max, min);
    Vector3 center = Vector3Add(min, Vector3Scale(size, 0.5f));

    Update(center, size);

    TraceLog(LOG_INFO,
             "Transformed collision calculated: center=(%.2f,%.2f,%.2f) size=(%.2f,%.2f,%.2f)",
             center.x, center.y, center.z, size.x, size.y, size.z);
}

Vector3 Collision::GetCenter() const
{
    return Vector3Add(m_min, Vector3Scale(Vector3Subtract(m_max, m_min), 0.5f));
}

Vector3 Collision::GetSize() const { return Vector3Subtract(m_max, m_min); }

// ================== BVH Methods Implementation ==================

void Collision::BuildBVH(Model *model) { BuildBVH(model, MatrixIdentity()); }

void Collision::BuildBVH(Model *model, const Matrix &transform)
{
    if (!model || model->meshCount == 0)
    {
        TraceLog(LOG_WARNING, "Invalid model provided for BVH construction");
        return;
    }

    // Extract triangles from model
    ExtractTrianglesFromModel(model, &transform);

    if (m_triangles.empty())
    {
        TraceLog(LOG_WARNING, "No triangles found in model for BVH construction");
        return;
    }

    // Build BVH tree
    std::vector<CollisionTriangle> triangleCopy = m_triangles; // Make a copy for recursive building
    m_bvhRoot = BuildBVHRecursive(triangleCopy);
    m_useBVH = true;

    TraceLog(LOG_INFO, "BVH constructed with %zu triangles", m_triangles.size());
}

void Collision::ExtractTrianglesFromModel(Model *model, const Matrix *transform)
{
    m_triangles.clear();

    for (int m = 0; m < model->meshCount; m++)
    {
        Mesh &mesh = model->meshes[m];

        // Handle indexed mesh
        if (mesh.indices)
        {
            // Indexed mesh
            for (int i = 0; i < mesh.triangleCount * 3; i += 3)
            {
                unsigned short i0 = mesh.indices[i];
                unsigned short i1 = mesh.indices[i + 1];
                unsigned short i2 = mesh.indices[i + 2];

                Vector3 v0 = {mesh.vertices[i0 * 3], mesh.vertices[i0 * 3 + 1],
                              mesh.vertices[i0 * 3 + 2]};
                Vector3 v1 = {mesh.vertices[i1 * 3], mesh.vertices[i1 * 3 + 1],
                              mesh.vertices[i1 * 3 + 2]};
                Vector3 v2 = {mesh.vertices[i2 * 3], mesh.vertices[i2 * 3 + 1],
                              mesh.vertices[i2 * 3 + 2]};

                if (transform)
                {
                    v0 = Vector3Transform(v0, *transform);
                    v1 = Vector3Transform(v1, *transform);
                    v2 = Vector3Transform(v2, *transform);
                }

                m_triangles.emplace_back(v0, v1, v2);
            }
        }
        else
        {
            // Non-indexed mesh
            for (int i = 0; i < mesh.vertexCount; i += 3)
            {
                Vector3 v0 = {mesh.vertices[i * 3], mesh.vertices[i * 3 + 1],
                              mesh.vertices[i * 3 + 2]};
                Vector3 v1 = {mesh.vertices[(i + 1) * 3], mesh.vertices[(i + 1) * 3 + 1],
                              mesh.vertices[(i + 1) * 3 + 2]};
                Vector3 v2 = {mesh.vertices[(i + 2) * 3], mesh.vertices[(i + 2) * 3 + 1],
                              mesh.vertices[(i + 2) * 3 + 2]};

                if (transform)
                {
                    v0 = Vector3Transform(v0, *transform);
                    v1 = Vector3Transform(v1, *transform);
                    v2 = Vector3Transform(v2, *transform);
                }

                m_triangles.emplace_back(v0, v1, v2);
            }
        }
    }
}

std::unique_ptr<BVHNode> Collision::BuildBVHRecursive(std::vector<CollisionTriangle> &triangles,
                                                      int depth)
{
    auto node = std::make_unique<BVHNode>();

    // Calculate bounding box for all triangles
    for (const auto &triangle : triangles)
    {
        Vector3 tMin = triangle.GetMin();
        Vector3 tMax = triangle.GetMax();

        node->minBounds.x = fminf(node->minBounds.x, tMin.x);
        node->minBounds.y = fminf(node->minBounds.y, tMin.y);
        node->minBounds.z = fminf(node->minBounds.z, tMin.z);

        node->maxBounds.x = fmaxf(node->maxBounds.x, tMax.x);
        node->maxBounds.y = fmaxf(node->maxBounds.y, tMax.y);
        node->maxBounds.z = fmaxf(node->maxBounds.z, tMax.z);
    }

    // Leaf node condition - use fewer triangles per leaf for more precision
    if (triangles.size() <= 7 || depth > 25)
    { // Max 2 triangles per leaf or max depth 25 for higher precision
        node->isLeaf = true;
        node->triangles = triangles;
        TraceLog(LOG_DEBUG, "Created leaf node at depth %d with %zu triangles", depth,
                 triangles.size());
        return node;
    }

    // Find best split axis (longest axis)
    Vector3 extent = Vector3Subtract(node->maxBounds, node->minBounds);
    int splitAxis = 0; // X axis
    if (extent.y > extent.x && extent.y > extent.z)
        splitAxis = 1; // Y axis
    if (extent.z > extent.x && extent.z > extent.y)
        splitAxis = 2; // Z axis

    // Sort triangles by center along split axis
    std::sort(triangles.begin(), triangles.end(),
              [splitAxis](const CollisionTriangle &a, const CollisionTriangle &b)
              {
                  Vector3 centerA = a.GetCenter();
                  Vector3 centerB = b.GetCenter();

                  switch (splitAxis)
                  {
                  case 0:
                      return centerA.x < centerB.x;
                  case 1:
                      return centerA.y < centerB.y;
                  case 2:
                      return centerA.z < centerB.z;
                  }
                  return false;
              });

    // Split triangles
    size_t mid = triangles.size() / 2;
    std::vector<CollisionTriangle> leftTriangles(triangles.begin(), triangles.begin() + mid);
    std::vector<CollisionTriangle> rightTriangles(triangles.begin() + mid, triangles.end());

    // Recursively build children
    node->left = BuildBVHRecursive(leftTriangles, depth + 1);
    node->right = BuildBVHRecursive(rightTriangles, depth + 1);

    return node;
}

bool Collision::IntersectsBVH(const Collision &other) const
{
    // // Debug: Log collision check details (temporary verbose logging)
    // TraceLog(LOG_INFO, "🔍 BVH Collision Check:");
    // TraceLog(LOG_INFO, "  This (Arena) BVH: %s, bounds: (%.2f,%.2f,%.2f) to (%.2f,%.2f,%.2f)",
    //          IsUsingBVH() ? "YES" : "NO", m_min.x, m_min.y, m_min.z, m_max.x, m_max.y, m_max.z);
    // TraceLog(LOG_INFO, "  Other (Player) bounds: (%.2f,%.2f,%.2f) to (%.2f,%.2f,%.2f)",
    //          other.m_min.x, other.m_min.y, other.m_min.z, other.m_max.x, other.m_max.y,
    //          other.m_max.z);

    // If this object has BVH, use it to test against other's AABB
    if (IsUsingBVH())
    {
        TraceLog(LOG_INFO, "Using Arena BVH vs Player AABB");
        bool result = IntersectsBVHRecursive(m_bvhRoot.get(), other.m_min, other.m_max);
        // if (result)
        // {
        //     TraceLog(LOG_INFO, "🎯 BVH intersection HIT!");
        //     TraceLog(LOG_INFO, "  🏟️  Arena bounds: Y %.2f to %.2f", m_min.y, m_max.y);
        //     TraceLog(LOG_INFO, "  🧍 Player bounds: Y %.2f to %.2f", other.m_min.y,
        //     other.m_max.y);
        // }
        // else
        // {
        //     TraceLog(LOG_INFO, "❌ No BVH intersection - checking Y overlap:");
        //     TraceLog(LOG_INFO, "  Arena Y: %.2f to %.2f", m_min.y, m_max.y);
        //     TraceLog(LOG_INFO, "  Player Y: %.2f to %.2f", other.m_min.y, other.m_max.y);
        // }
        return result;
    }

    // If other object has BVH, use it to test against this AABB
    if (other.IsUsingBVH())
    {
        TraceLog(LOG_DEBUG, "Using other object's BVH vs this AABB");
        bool result = other.IntersectsBVHRecursive(other.m_bvhRoot.get(), m_min, m_max);
        if (result)
        {
            TraceLog(LOG_INFO, "🎯 BVH intersection HIT!");
        }
        return result;
    }

    // Neither has BVH, fallback to AABB
    TraceLog(LOG_DEBUG, "Both objects use AABB, falling back to standard intersection");
    return Intersects(other);
}

bool Collision::IntersectsBVHRecursive(const BVHNode *node, const Vector3 &otherMin,
                                       const Vector3 &otherMax) const
{
    if (!node)
        return false;

    // Check if bounding boxes intersect
    if (!node->Intersects(otherMin, otherMax))
        return false;

    if (node->isLeaf)
    {
        // For leaf nodes, check if ANY triangle intersects the AABB
        TraceLog(LOG_DEBUG, "Checking leaf node with %zu triangles", node->triangles.size());

        for (const auto &triangle : node->triangles)
        {
            // Use precise triangle-AABB intersection algorithm
            if (TriangleIntersectsAABB(triangle, otherMin, otherMax))
            {
                TraceLog(LOG_DEBUG, "Precise Triangle-AABB intersection found!");
                return true;
            }
        }

        // No triangles intersect
        TraceLog(LOG_DEBUG, "No triangle-AABB intersections in leaf");
        return false;
    }

    // Check children
    return IntersectsBVHRecursive(node->left.get(), otherMin, otherMax) ||
           IntersectsBVHRecursive(node->right.get(), otherMin, otherMax);
}

bool Collision::RaycastBVH(const CollisionRay &ray, float &distance, Vector3 &hitPoint,
                           Vector3 &hitNormal) const
{
    if (!IsUsingBVH())
        return false;

    float closestT = std::numeric_limits<float>::max();
    Vector3 closestNormal = {0, 1, 0};
    bool hit = false;

    std::function<void(const BVHNode *)> traverse = [&](const BVHNode *node)
    {
        if (!node)
            return;

        float t;
        if (!node->IntersectsRay(ray, t) || t >= closestT)
            return;

        if (node->isLeaf)
        {
            // Test ray against triangles in leaf
            for (const auto &triangle : node->triangles)
            {
                float triangleT;
                if (triangle.Intersects(ray, triangleT) && triangleT < closestT)
                {
                    closestT = triangleT;
                    closestNormal = triangle.normal;
                    hit = true;
                }
            }
        }
        else
        {
            // Traverse children
            traverse(node->left.get());
            traverse(node->right.get());
        }
    };

    traverse(m_bvhRoot.get());

    if (hit)
    {
        distance = closestT;
        hitPoint = Vector3Add(ray.origin, Vector3Scale(ray.direction, closestT));
        hitNormal = closestNormal;
    }

    return hit;
}

bool Collision::ContainsBVH(const Vector3 &point) const
{
    if (!IsUsingBVH())
        return Contains(point);

    // Simple point-in-box test for now
    // For true point-in-mesh, we'd need to cast a ray and count intersections
    return Contains(point);
}

// Precise Triangle-AABB intersection using Separating Axis Theorem (SAT)
bool Collision::TriangleIntersectsAABB(const CollisionTriangle &triangle, const Vector3 &boxMin,
                                       const Vector3 &boxMax) const
{
    Vector3 boxCenter = Vector3Scale(Vector3Add(boxMin, boxMax), 0.5f);
    Vector3 boxExtents = Vector3Scale(Vector3Subtract(boxMax, boxMin), 0.5f);

    // Move triangle to box coordinate system
    Vector3 v0 = Vector3Subtract(triangle.v0, boxCenter);
    Vector3 v1 = Vector3Subtract(triangle.v1, boxCenter);
    Vector3 v2 = Vector3Subtract(triangle.v2, boxCenter);

    // Triangle edges
    Vector3 edge0 = Vector3Subtract(v1, v0);
    Vector3 edge1 = Vector3Subtract(v2, v1);
    Vector3 edge2 = Vector3Subtract(v0, v2);

    // AABB axes (X, Y, Z)
    Vector3 axes[3] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};

    // Test triangle normal
    Vector3 normal = Vector3CrossProduct(edge0, edge1);
    float normalLen = Vector3Length(normal);
    if (normalLen > 0.0001f)
    {
        normal = Vector3Scale(normal, 1.0f / normalLen);

        // Project triangle onto normal
        float triProj = Vector3DotProduct(v0, normal);
        float triMin = triProj, triMax = triProj;

        float proj1 = Vector3DotProduct(v1, normal);
        float proj2 = Vector3DotProduct(v2, normal);
        triMin = fminf(triMin, fminf(proj1, proj2));
        triMax = fmaxf(triMax, fmaxf(proj1, proj2));

        // Project box onto normal
        float boxRadius = fabsf(boxExtents.x * normal.x) + fabsf(boxExtents.y * normal.y) +
                          fabsf(boxExtents.z * normal.z);

        if (triMax < -boxRadius || triMin > boxRadius)
        {
            return false; // Separated by triangle normal
        }
    }

    // Test box axes
    for (int i = 0; i < 3; i++)
    {
        Vector3 axis = axes[i];

        // Project triangle vertices
        float p0 = Vector3DotProduct(v0, axis);
        float p1 = Vector3DotProduct(v1, axis);
        float p2 = Vector3DotProduct(v2, axis);

        float triMin = fminf(p0, fminf(p1, p2));
        float triMax = fmaxf(p0, fmaxf(p1, p2));

        // Box projection is just the extent
        float boxExtent = (i == 0) ? boxExtents.x : (i == 1) ? boxExtents.y : boxExtents.z;

        if (triMax < -boxExtent || triMin > boxExtent)
        {
            return false; // Separated by box axis
        }
    }

    // Test edge cross products (9 axes: 3 triangle edges × 3 box axes)
    Vector3 edges[3] = {edge0, edge1, edge2};

    for (int i = 0; i < 3; i++)
    {
        for (int j = 0; j < 3; j++)
        {
            Vector3 axis = Vector3CrossProduct(edges[i], axes[j]);
            float axisLenSq = Vector3DotProduct(axis, axis);

            if (axisLenSq < 0.0001f)
                continue; // Skip degenerate axes

            // Project triangle
            float p0 = Vector3DotProduct(v0, axis);
            float p1 = Vector3DotProduct(v1, axis);
            float p2 = Vector3DotProduct(v2, axis);

            float triMin = fminf(p0, fminf(p1, p2));
            float triMax = fmaxf(p0, fmaxf(p1, p2));

            // Project box
            float boxRadius = fabsf(boxExtents.x * axis.x) + fabsf(boxExtents.y * axis.y) +
                              fabsf(boxExtents.z * axis.z);

            if (triMax < -boxRadius || triMin > boxRadius)
            {
                return false; // Separated by edge cross product
            }
        }
    }

    return true; // No separating axis found - intersection detected
}

size_t Collision::GetTriangleCount() const { return m_triangles.size(); }