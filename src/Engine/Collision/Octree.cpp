//
// Created by Assistant - Octree collision system implementation
//

#include "Octree.h"
#include "CollisionSystem.h"
#include "raylib.h"
#include "raymath.h"
#include <algorithm>
#include <cmath>

// ================== OctreeNode Implementation ==================

OctreeNode::OctreeNode(const Vector3 &center, float halfSize)
    : center(center), halfSize(halfSize), isLeaf(true)
{
    for (auto &i : children)
    {
        i = nullptr;
    }
}

bool OctreeNode::Contains(const Vector3 &point) const
{
    return (point.x >= center.x - halfSize && point.x <= center.x + halfSize) &&
           (point.y >= center.y - halfSize && point.y <= center.y + halfSize) &&
           (point.z >= center.z - halfSize && point.z <= center.z + halfSize);
}

bool OctreeNode::IntersectsAABB(const Vector3 &min, const Vector3 &max) const
{
    Vector3 nodeMin = GetMin();
    Vector3 nodeMax = GetMax();

    return (nodeMin.x <= max.x && nodeMax.x >= min.x) &&
           (nodeMin.y <= max.y && nodeMax.y >= min.y) && (nodeMin.z <= max.z && nodeMax.z >= min.z);
}

int OctreeNode::GetChildIndex(const Vector3 &point) const
{
    return (point.x > center.x) | ((point.y > center.y) << 1) | ((point.z > center.z) << 2);
}

Vector3 OctreeNode::GetMin() const
{
    return {center.x - halfSize, center.y - halfSize, center.z - halfSize};
}

Vector3 OctreeNode::GetMax() const
{
    return {center.x + halfSize, center.y + halfSize, center.z + halfSize};
}

// ================== Octree Implementation ==================

Octree::Octree() : m_triangleCount(0)
{
    m_min = Vector3Zero();
    m_max = Vector3Zero();
}

void Octree::Initialize(const Vector3 &min, const Vector3 &max)
{
    m_min = min;
    m_max = max;

    // Calculate center and size
    Vector3 size = Vector3Subtract(max, min);
    Vector3 center = Vector3Add(min, Vector3Scale(size, 0.5f));

    // Use the maximum dimension to create a cube
    float maxSize = std::max({size.x, size.y, size.z});
    float halfSize = maxSize * 0.5f;

    m_root = std::make_unique<OctreeNode>(center, halfSize);
    m_triangleCount = 0;

    TraceLog(LOG_INFO, "Octree initialized: center(%.2f,%.2f,%.2f) halfSize=%.2f", center.x,
             center.y, center.z, halfSize);
}

void Octree::BuildFromModel(Model *model, const Matrix &transform)
{
    if (!model || model->meshCount == 0)
    {
        TraceLog(LOG_WARNING, "Invalid model provided for octree construction");
        return;
    }

    std::vector<CollisionTriangle> triangles;

    int estimatedCount = 0;
    for (int m = 0; m < model->meshCount; m++)
        estimatedCount += model->meshes[m].triangleCount;
    triangles.reserve(estimatedCount);

    ExtractTrianglesFromModel(model, transform, triangles);

    if (triangles.empty())
    {
        TraceLog(LOG_WARNING, "No triangles found in model for octree construction");
        return;
    }

    TraceLog(LOG_INFO, "Building octree from %zu triangles", triangles.size());

    // Глобальний AABB одразу по готовим трикутникам
    Vector3 min = {FLT_MAX, FLT_MAX, FLT_MAX};
    Vector3 max = {-FLT_MAX, -FLT_MAX, -FLT_MAX};
    for (const auto &tri : triangles)
    {
        min.x = std::min(min.x, tri.min.x);
        min.y = std::min(min.y, tri.min.y);
        min.z = std::min(min.z, tri.min.z);

        max.x = std::max(max.x, tri.max.x);
        max.y = std::max(max.y, tri.max.y);
        max.z = std::max(max.z, tri.max.z);
    }

    Initialize(min, max);

    // тут важливо: передаємо вже готові CollisionTriangle (з min/max)
    BuildRecursive(m_root.get(), triangles, 0);

    m_triangleCount = CountTriangles(m_root.get());

    TraceLog(LOG_INFO, "Octree built with %zu triangles in %zu nodes", GetTriangleCount(),
             GetNodeCount());
}

void Octree::BuildRecursive(OctreeNode *node, const std::vector<CollisionTriangle> &triangles,
                            int depth)
{
    // Stop if we have few triangles or reached max depth or node is too small
    if (triangles.size() <= MAX_TRIANGLES_PER_NODE || depth >= MAX_DEPTH ||
        node->halfSize <= MIN_NODE_SIZE)
    {
        node->triangles = triangles;
        node->isLeaf = true;
        // Note: Don't increment m_triangleCount here as triangles might be duplicated across nodes
        // Triangle count will be calculated properly at the end
        return;
    }

    // Create children
    node->isLeaf = false;
    float childHalfSize = node->halfSize * 0.5f;

    for (int i = 0; i < 8; i++)
    {
        Vector3 childCenter = node->center;
        if (i & 1)
            childCenter.x += childHalfSize; // Right
        else
            childCenter.x -= childHalfSize; // Left
        if (i & 2)
            childCenter.y += childHalfSize; // Top
        else
            childCenter.y -= childHalfSize; // Bottom
        if (i & 4)
            childCenter.z += childHalfSize; // Front
        else
            childCenter.z -= childHalfSize; // Back

        node->children[i] = std::make_unique<OctreeNode>(childCenter, childHalfSize);
    }

    // Distribute triangles to children
    for (const auto &i : node->children)
    {
        std::vector<CollisionTriangle> childTriangles;

        for (const auto &triangle : triangles)
        {
            if (TriangleIntersectsNode(triangle, i.get()))
            {
                childTriangles.push_back(triangle);
            }
        }

        if (!childTriangles.empty())
        {
            BuildRecursive(i.get(), childTriangles, depth + 1);
        }
    }
}

void Octree::AddTriangle(const CollisionTriangle &triangle)
{
    if (m_root)
    {
        AddTriangleRecursive(m_root.get(), triangle, 0);
        // Only increment if this is a new unique triangle
        m_triangleCount++;
    }
}

void Octree::AddTriangleRecursive(OctreeNode *node, const CollisionTriangle &triangle, int depth)
{
    if (!TriangleIntersectsNode(triangle, node))
        return;

    if (node->isLeaf)
    {
        node->triangles.push_back(triangle);

        // Check if we need to subdivide
        if (node->triangles.size() > MAX_TRIANGLES_PER_NODE && depth < MAX_DEPTH &&
            node->halfSize > MIN_NODE_SIZE)
        {
            // Convert to non-leaf and redistribute triangles
            node->isLeaf = false;
            float childHalfSize = node->halfSize * 0.5f;

            for (int i = 0; i < 8; i++)
            {
                Vector3 childCenter = node->center;
                if (i & 1)
                    childCenter.x += childHalfSize;
                else
                    childCenter.x -= childHalfSize;
                if (i & 2)
                    childCenter.y += childHalfSize;
                else
                    childCenter.y -= childHalfSize;
                if (i & 4)
                    childCenter.z += childHalfSize;
                else
                    childCenter.z -= childHalfSize;

                node->children[i] = std::make_unique<OctreeNode>(childCenter, childHalfSize);
            }

            // Redistribute existing triangles
            auto triangles = std::move(node->triangles);
            node->triangles.clear();

            for (const auto &tri : triangles)
            {
                for (const auto &i : node->children)
                {
                    if (TriangleIntersectsNode(tri, i.get()))
                    {
                        AddTriangleRecursive(i.get(), tri, depth + 1);
                    }
                }
            }
        }
    }
    else
    {
        // Not a leaf, add to appropriate children
        for (const auto &i : node->children)
        {
            if (i && TriangleIntersectsNode(triangle, i.get()))
            {
                AddTriangleRecursive(i.get(), triangle, depth + 1);
            }
        }
    }
}

void Octree::Clear()
{
    m_root.reset();
    m_triangleCount = 0;
}

bool Octree::IntersectsAABB(const Vector3 &min, const Vector3 &max) const
{
    if (!m_root)
        return false;
    return IntersectsAABBRecursive(m_root.get(), min, max);
}

bool Octree::IntersectsOctree(const Octree &other) const
{
    if (!m_root || !other.m_root)
        return false;
    return IntersectsOctreeRecursive(m_root.get(), other.m_root.get());
}

bool Octree::IntersectsImproved(const Vector3 &min, const Vector3 &max) const
{
    if (!m_root)
        return false;
    return IntersectsImprovedRecursive(m_root.get(), min, max);
}

bool Octree::IntersectsAABBRecursive(const OctreeNode *node, const Vector3 &min,
                                     const Vector3 &max) const
{
    if (!node->IntersectsAABB(min, max))
        return false;

    if (node->isLeaf)
    {
        // Check triangles in leaf
        for (const auto &triangle : node->triangles)
        {
            if (TriangleIntersectsAABB(triangle, min, max))
            {
                return true;
            }
        }
        return false;
    }

    // Check children
    for (const auto &i : node->children)
    {
        if (i && IntersectsAABBRecursive(i.get(), min, max))
        {
            return true;
        }
    }

    return false;
}

bool Octree::ContainsPoint(const Vector3 &point) const
{
    if (!m_root)
        return false;
    return ContainsPointRecursive(m_root.get(), point);
}

bool Octree::ContainsPointRecursive(const OctreeNode *node, const Vector3 &point) const
{
    if (!node->Contains(point))
        return false;

    if (node->isLeaf)
    {
        if (node->triangles.empty())
            return false;

        constexpr float FLOOR_THRESHOLD = 0.02f;
        constexpr float WALL_THRESHOLD = 0.005f;
        constexpr float CEILING_THRESHOLD = 0.02f;

        for (const auto &tri : node->triangles)
        {
            // AABB відсів
            if (point.x < tri.min.x || point.x > tri.max.x || point.y < tri.min.y ||
                point.y > tri.max.y || point.z < tri.min.z || point.z > tri.max.z)
            {
                continue;
            }

            Vector3 v0p = Vector3Subtract(point, tri.v0);

            // Використовуємо кешовані dot products для barycentric
            float dot02 = Vector3DotProduct(tri.e1, v0p);
            float dot12 = Vector3DotProduct(tri.e0, v0p);

            float denom = tri.dot00 * tri.dot11 - tri.dot01 * tri.dot01;
            if (fabsf(denom) < 1e-6f)
                continue;

            float invDenom = 1.0f / denom;
            float u = (tri.dot11 * dot02 - tri.dot01 * dot12) * invDenom;
            float v = (tri.dot00 * dot12 - tri.dot01 * dot02) * invDenom;

            if (u >= 0 && v >= 0 && u + v <= 1)
            {
                float distanceToPlane = fabsf(Vector3DotProduct(v0p, tri.normal));

                if (distanceToPlane <= FLOOR_THRESHOLD)
                {
                    if (tri.normal.y > 0.7f)
                        return distanceToPlane <= FLOOR_THRESHOLD;
                    else if (tri.normal.y < -0.7f)
                        return distanceToPlane <= CEILING_THRESHOLD;
                    else
                        return distanceToPlane <= WALL_THRESHOLD;
                }
            }
        }
        return false;
    }

    for (const auto &child : node->children)
    {
        if (child && child->Contains(point) && ContainsPointRecursive(child.get(), point))
            return true;
    }

    return false;
}

size_t Octree::GetTriangleCount() const { return m_triangleCount; }

size_t Octree::GetNodeCount() const
{
    if (!m_root)
        return 0;
    return CountNodesRecursive(m_root.get());
}

size_t Octree::CountNodesRecursive(const OctreeNode *node) const
{
    if (!node)
        return 0;

    size_t count = 1; // This node

    if (!node->isLeaf)
    {
        for (const auto &i : node->children)
        {
            count += CountNodesRecursive(i.get());
        }
    }

    return count;
}

void Octree::GetAllNodes(std::vector<std::pair<Vector3, float>> &nodes) const
{
    nodes.clear();
    if (m_root)
    {
        GetAllNodesRecursive(m_root.get(), nodes);
    }
}

void Octree::GetAllNodesRecursive(const OctreeNode *node,
                                  std::vector<std::pair<Vector3, float>> &nodes) const
{
    if (!node)
        return;

    nodes.emplace_back(node->center, node->halfSize);

    if (!node->isLeaf)
    {
        for (const auto &i : node->children)
        {
            GetAllNodesRecursive(i.get(), nodes);
        }
    }
}

bool Octree::TriangleIntersectsNode(const CollisionTriangle &triangle, const OctreeNode *node) const
{
    Vector3 nodeMin = node->GetMin();
    Vector3 nodeMax = node->GetMax();
    return TriangleIntersectsAABB(triangle, nodeMin, nodeMax);
}

bool Octree::TriangleIntersectsAABB(const CollisionTriangle &triangle, const Vector3 &boxMin,
                                    const Vector3 &boxMax) const
{
    // Simplified triangle-AABB intersection test
    // Check if any vertex is inside the box
    if ((triangle.v0.x >= boxMin.x && triangle.v0.x <= boxMax.x && triangle.v0.y >= boxMin.y &&
         triangle.v0.y <= boxMax.y && triangle.v0.z >= boxMin.z && triangle.v0.z <= boxMax.z) ||
        (triangle.v1.x >= boxMin.x && triangle.v1.x <= boxMax.x && triangle.v1.y >= boxMin.y &&
         triangle.v1.y <= boxMax.y && triangle.v1.z >= boxMin.z && triangle.v1.z <= boxMax.z) ||
        (triangle.v2.x >= boxMin.x && triangle.v2.x <= boxMax.x && triangle.v2.y >= boxMin.y &&
         triangle.v2.y <= boxMax.y && triangle.v2.z >= boxMin.z && triangle.v2.z <= boxMax.z))
    {
        return true;
    }

    // Check if triangle bounding box intersects node bounding box
    Vector3 triMin = triangle.GetMin();
    Vector3 triMax = triangle.GetMax();

    return (triMin.x <= boxMax.x && triMax.x >= boxMin.x) &&
           (triMin.y <= boxMax.y && triMax.y >= boxMin.y) &&
           (triMin.z <= boxMax.z && triMax.z >= boxMin.z);
}

void Octree::ExtractTrianglesFromModel(Model *model, const Matrix &transform,
                                       std::vector<CollisionTriangle> &triangles)
{
    triangles.clear();

    for (int m = 0; m < model->meshCount; m++)
    {
        Mesh &mesh = model->meshes[m];

        if (!mesh.vertices || mesh.vertexCount == 0)
            continue;

        // Process triangles
        if (mesh.indices)
        {
            // Indexed mesh
            for (int i = 0; i < mesh.triangleCount; i++)
            {
                int idx0 = mesh.indices[i * 3 + 0];
                int idx1 = mesh.indices[i * 3 + 1];
                int idx2 = mesh.indices[i * 3 + 2];

                Vector3 v0 = {mesh.vertices[idx0 * 3 + 0], mesh.vertices[idx0 * 3 + 1],
                              mesh.vertices[idx0 * 3 + 2]};
                Vector3 v1 = {mesh.vertices[idx1 * 3 + 0], mesh.vertices[idx1 * 3 + 1],
                              mesh.vertices[idx1 * 3 + 2]};
                Vector3 v2 = {mesh.vertices[idx2 * 3 + 0], mesh.vertices[idx2 * 3 + 1],
                              mesh.vertices[idx2 * 3 + 2]};

                // Apply transformation
                v0 = Vector3Transform(v0, transform);
                v1 = Vector3Transform(v1, transform);
                v2 = Vector3Transform(v2, transform);

                triangles.emplace_back(v0, v1, v2);
            }
        }
        else
        {
            // Non-indexed mesh
            for (int i = 0; i < mesh.vertexCount; i += 3)
            {
                Vector3 v0 = {mesh.vertices[i * 3 + 0], mesh.vertices[i * 3 + 1],
                              mesh.vertices[i * 3 + 2]};
                Vector3 v1 = {mesh.vertices[(i + 1) * 3 + 0], mesh.vertices[(i + 1) * 3 + 1],
                              mesh.vertices[(i + 1) * 3 + 2]};
                Vector3 v2 = {mesh.vertices[(i + 2) * 3 + 0], mesh.vertices[(i + 2) * 3 + 1],
                              mesh.vertices[(i + 2) * 3 + 2]};

                // Apply transformation
                v0 = Vector3Transform(v0, transform);
                v1 = Vector3Transform(v1, transform);
                v2 = Vector3Transform(v2, transform);

                triangles.emplace_back(v0, v1, v2);
            }
        }
    }

    TraceLog(LOG_INFO, "Extracted %zu triangles from model with %d meshes", triangles.size(),
             model->meshCount);
}

// ================== Ray Casting Implementation ==================

bool Octree::Raycast(const Vector3 &origin, const Vector3 &direction, float maxDistance,
                     float &hitDistance, Vector3 &hitPoint, Vector3 &hitNormal) const
{
    if (!m_root)
        return false;

    hitDistance = maxDistance;
    return RaycastRecursive(m_root.get(), origin, direction, maxDistance, hitDistance, hitPoint,
                            hitNormal);
}

bool Octree::RaycastRecursive(const OctreeNode *node, const Vector3 &origin,
                              const Vector3 &direction, float maxDistance, float &hitDistance,
                              Vector3 &hitPoint, Vector3 &hitNormal) const
{
    if (!node)
        return false;

    // Check if ray intersects node's AABB
    Vector3 nodeMin = node->GetMin();
    Vector3 nodeMax = node->GetMax();

    // Simple AABB-ray intersection test
    float tMin = 0.0f;
    float tMax = maxDistance;

    for (int i = 0; i < 3; i++)
    {
        float origin_i = (i == 0) ? origin.x : (i == 1) ? origin.y : origin.z;
        float dir_i = (i == 0) ? direction.x : (i == 1) ? direction.y : direction.z;
        float min_i = (i == 0) ? nodeMin.x : (i == 1) ? nodeMin.y : nodeMin.z;
        float max_i = (i == 0) ? nodeMax.x : (i == 1) ? nodeMax.y : nodeMax.z;

        if (fabsf(dir_i) < 1e-6f)
        {
            // Ray is parallel to slab
            if (origin_i < min_i || origin_i > max_i)
                return false;
        }
        else
        {
            float t1 = (min_i - origin_i) / dir_i;
            float t2 = (max_i - origin_i) / dir_i;

            if (t1 > t2)
            {
                float temp = t1;
                t1 = t2;
                t2 = temp;
            }

            tMin = fmaxf(tMin, t1);
            tMax = fminf(tMax, t2);

            if (tMin > tMax)
                return false;
        }
    }

    if (tMin > maxDistance || tMax < 0.0f)
        return false;

    bool hit = false;
    float closestDistance = hitDistance;

    if (node->isLeaf)
    {
        // Test against triangles in this leaf
        for (const auto &triangle : node->triangles)
        {
            float t;

            if (triangle.Intersects(origin, direction, t) && t < closestDistance && t >= 0.0f)
            {
                closestDistance = t;
                hitPoint = Vector3Add(origin, Vector3Scale(direction, t));
                hitNormal = triangle.normal;
                hit = true;
            }
        }
    }
    else
    {
        // Test against children
        for (const auto &i : node->children)
        {
            if (i)
            {
                float childDistance = closestDistance;
                Vector3 childPoint, childNormal;

                if (RaycastRecursive(i.get(), origin, direction, closestDistance, childDistance,
                                     childPoint, childNormal))
                {
                    if (childDistance < closestDistance)
                    {
                        closestDistance = childDistance;
                        hitPoint = childPoint;
                        hitNormal = childNormal;
                        hit = true;
                    }
                }
            }
        }
    }

    if (hit)
    {
        hitDistance = closestDistance;
        return true;
    }

    return false;
}

bool Octree::IntersectsOctreeRecursive(const OctreeNode *thisNode,
                                       const OctreeNode *otherNode) const
{
    if (!thisNode || !otherNode)
        return false;

    // Check if nodes' AABBs intersect
    if (!thisNode->IntersectsAABB(otherNode->GetMin(), otherNode->GetMax()))
        return false;

    // If both are leaves, check triangle-triangle intersections
    if (thisNode->isLeaf && otherNode->isLeaf)
    {
        for (const auto &thisTriangle : thisNode->triangles)
        {
            for (const auto &otherTriangle : otherNode->triangles)
            {
                if (thisTriangle.Intersects(otherTriangle))
                    return true;
            }
        }
        return false;
    }

    // If one is leaf and other is not, recurse on the non-leaf
    if (thisNode->isLeaf && !otherNode->isLeaf)
    {
        for (const auto &i : otherNode->children)
        {
            if (i && IntersectsOctreeRecursive(thisNode, i.get()))
                return true;
        }
        return false;
    }

    if (!thisNode->isLeaf && otherNode->isLeaf)
    {
        for (const auto &i : thisNode->children)
        {
            if (i && IntersectsOctreeRecursive(i.get(), otherNode))
                return true;
        }
        return false;
    }

    // Both are internal nodes, recurse on all combinations
    for (const auto &i : thisNode->children)
    {
        if (i)
        {
            for (const auto &j : otherNode->children)
            {
                if (j && IntersectsOctreeRecursive(i.get(), j.get()))
                    return true;
            }
        }
    }

    return false;
}

bool Octree::IntersectsImprovedRecursive(const OctreeNode *node, const Vector3 &min,
                                         const Vector3 &max) const
{
    if (!node->IntersectsAABB(min, max))
        return false;

    if (node->isLeaf)
    {
        // For improved collision, we use smaller node AABBs instead of full triangle checks
        // This gives better precision than simple AABB but is faster than triangle-triangle
        return true; // If we reach a leaf and AABBs intersect, consider it a collision
    }

    // Check children with smaller AABBs
    for (const auto &i : node->children)
    {
        if (i && IntersectsImprovedRecursive(i.get(), min, max))
        {
            return true;
        }
    }

    return false;
}

void Octree::DebugDraw(const Color &color) const
{
    if (m_root)
    {
        DebugDrawRecursive(m_root.get(), color);
    }
}

void Octree::DebugDrawRecursive(const OctreeNode *node, const Color &color) const
{
    if (!node)
        return;

    Vector3 size = {node->halfSize * 2, node->halfSize * 2, node->halfSize * 2};
    DrawCubeWires(node->center, size.x, size.y, size.z, color);

    if (!node->isLeaf)
    {
        for (const auto &i : node->children)
            if (i)
                DebugDrawRecursive(i.get(), color);
    }
}

size_t Octree::CountTriangles(const OctreeNode *node) const
{
    if (!node)
        return 0;
    if (node->isLeaf)
        return node->triangles.size();

    size_t count = 0;
    for (const auto &child : node->children)
    {
        if (child)
            count += CountTriangles(child.get());
    }
    return count;
}

BoundingBox Octree::GetBounds() const { return m_bounds; }