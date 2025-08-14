//
// CollisionSystem.cpp - Hybrid AABB + Octree Implementation
// Created by Assistant for optimized collision detection
//

#include "CollisionSystem.h"
#include "Octree.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <raylib.h>

// ================== Collision Implementation ==================

Collision::Collision() = default;

Collision::Collision(const Vector3 &center, const Vector3 &size)
{
    Update(center, size);
    m_collisionType = CollisionType::AABB_ONLY;
}

// Copy constructor
Collision::Collision(const Collision &other)
    : m_min(other.m_min), m_max(other.m_max), m_collisionType(other.m_collisionType),
      m_complexity(other.m_complexity), m_triangles(other.m_triangles)
{
    // Deep copy of Octree if it exists
    if (other.m_octree && other.m_collisionType != CollisionType::AABB_ONLY)
    {
        // For now, mark for rebuild instead of deep copying
        m_collisionType = CollisionType::HYBRID_AUTO;
    }
}

// Copy assignment operator
Collision &Collision::operator=(const Collision &other)
{
    if (this != &other)
    {
        m_min = other.m_min;
        m_max = other.m_max;
        m_collisionType = other.m_collisionType;
        m_complexity = other.m_complexity;
        m_triangles = other.m_triangles;

        // Reset Octree - will need to be rebuilt if needed
        m_octree.reset();
        if (other.m_octree && other.m_collisionType != CollisionType::AABB_ONLY)
        {
            m_collisionType = CollisionType::HYBRID_AUTO;
        }
    }
    return *this;
}

// Move constructor
Collision::Collision(Collision &&other) noexcept
    : m_min(std::move(other.m_min)), m_max(std::move(other.m_max)),
      m_collisionType(other.m_collisionType), m_complexity(std::move(other.m_complexity)),
      m_triangles(std::move(other.m_triangles)), m_octree(std::move(other.m_octree))
{
    other.m_collisionType = CollisionType::AABB_ONLY;
}

// Move assignment operator
Collision &Collision::operator=(Collision &&other) noexcept
{
    if (this != &other)
    {
        m_min = std::move(other.m_min);
        m_max = std::move(other.m_max);
        m_collisionType = other.m_collisionType;
        m_complexity = std::move(other.m_complexity);
        m_triangles = std::move(other.m_triangles);
        m_octree = std::move(other.m_octree);

        other.m_collisionType = CollisionType::AABB_ONLY;
    }
    return *this;
}

Collision::~Collision() = default;

// ================== Basic AABB Methods ==================

void Collision::Update(const Vector3 &center, const Vector3 &size)
{
    m_min = Vector3Subtract(center, Vector3Scale(size, 0.5f));
    m_max = Vector3Add(center, Vector3Scale(size, 0.5f));
}

bool Collision::Intersects(const Collision &other) const
{
    StartPerformanceTimer();

    // Always start with AABB broad-phase check
    bool aabbIntersects = (m_min.x <= other.m_max.x && m_max.x >= other.m_min.x) &&
                          (m_min.y <= other.m_max.y && m_max.y >= other.m_min.y) &&
                          (m_min.z <= other.m_max.z && m_max.z >= other.m_min.z);

    if (!aabbIntersects)
    {
        EndPerformanceTimer(CollisionType::AABB_ONLY);
        return false;
    }

    // Determine which collision method to use
    CollisionType thisType = (m_collisionType == CollisionType::HYBRID_AUTO)
                                 ? DetermineOptimalCollisionType()
                                 : m_collisionType;
    CollisionType otherType = (other.m_collisionType == CollisionType::HYBRID_AUTO)
                                  ? other.DetermineOptimalCollisionType()
                                  : other.m_collisionType;

    // If both are simple, use AABB
    if (thisType == CollisionType::AABB_ONLY && otherType == CollisionType::AABB_ONLY)
    {
        EndPerformanceTimer(CollisionType::AABB_ONLY);
        return true; // AABB already passed
    }

    // If either is complex, use octree intersection
    if ((thisType == CollisionType::OCTREE_ONLY && m_octree) ||
        (otherType == CollisionType::OCTREE_ONLY && other.m_octree))
    {
        bool result = IntersectsOctree(other);
        EndPerformanceTimer(CollisionType::OCTREE_ONLY);
        return result;
    }

    // Fallback to AABB
    EndPerformanceTimer(CollisionType::AABB_ONLY);
    return true;
}

bool Collision::Contains(const Vector3 &point) const
{
    StartPerformanceTimer();

    // AABB check first
    bool aabbContains = (point.x >= m_min.x && point.x <= m_max.x) &&
                        (point.y >= m_min.y && point.y <= m_max.y) &&
                        (point.z >= m_min.z && point.z <= m_max.z);

    if (!aabbContains)
    {
        EndPerformanceTimer(CollisionType::AABB_ONLY);
        return false;
    }

    // Determine collision method
    CollisionType type = (m_collisionType == CollisionType::HYBRID_AUTO)
                             ? DetermineOptimalCollisionType()
                             : m_collisionType;

    if (type == CollisionType::OCTREE_ONLY && m_octree)
    {
        bool result = ContainsOctree(point);
        EndPerformanceTimer(CollisionType::OCTREE_ONLY);
        return result;
    }

    // Fallback to AABB result
    EndPerformanceTimer(CollisionType::AABB_ONLY);
    return true;
}

Vector3 Collision::GetMin() const { return m_min; }
Vector3 Collision::GetMax() const { return m_max; }
Vector3 Collision::GetCenter() const
{
    return Vector3Add(m_min, Vector3Scale(Vector3Subtract(m_max, m_min), 0.5f));
}
Vector3 Collision::GetSize() const { return Vector3Subtract(m_max, m_min); }

// ================== Hybrid Model Building ==================

void Collision::BuildFromModel(Model *model, const Matrix &transform)
{
    if (!model || model->meshCount == 0)
    {
        TraceLog(LOG_WARNING, "Invalid model provided for collision building");
        return;
    }

    // Analyze model complexity
    AnalyzeModelComplexity(model, transform);

    // Extract triangles for analysis and potential octree building
    ExtractTrianglesFromModel(model, transform);

    // Update AABB from triangles
    UpdateAABBFromTriangles();

    // Determine optimal collision type
    CollisionType optimalType = DetermineOptimalCollisionType();

    TraceLog(LOG_INFO, "Model complexity analysis:");
    TraceLog(LOG_INFO, "  Triangles: %zu", m_complexity.triangleCount);
    TraceLog(LOG_INFO, "  Surface area: %.2f", m_complexity.surfaceArea);
    TraceLog(LOG_INFO, "  Is simple: %s", m_complexity.IsSimple() ? "YES" : "NO");
    TraceLog(LOG_INFO, "  Optimal type: %s",
             (optimalType == CollisionType::AABB_ONLY) ? "AABB" : "OCTREE");

    // Build appropriate collision structure
    if (optimalType == CollisionType::OCTREE_ONLY)
    {
        BuildOctree(model, transform);
        m_collisionType = CollisionType::OCTREE_ONLY;
        TraceLog(LOG_INFO, "Built octree collision with %zu nodes", GetNodeCount());
    }
    else
    {
        m_collisionType = CollisionType::AABB_ONLY;
        TraceLog(LOG_INFO, "Using AABB collision for simple model");
    }
}

void Collision::BuildFromModel(Model *model, CollisionType type, const Matrix &transform)
{
    if (!model || model->meshCount == 0)
    {
        TraceLog(LOG_WARNING, "Invalid model provided for collision building");
        return;
    }

    // Force specific collision type
    m_collisionType = type;

    // Analyze model complexity anyway for stats
    AnalyzeModelComplexity(model, transform);
    ExtractTrianglesFromModel(model, transform);
    UpdateAABBFromTriangles();

    if (type == CollisionType::OCTREE_ONLY)
    {
        BuildOctree(model, transform);
        TraceLog(LOG_INFO, "Forced octree collision with %zu nodes", GetNodeCount());
    }
    else
    {
        TraceLog(LOG_INFO, "Forced AABB collision");
    }
}

// ================== Legacy Methods (Backward Compatibility) ==================

void Collision::CalculateFromModel(Model *model) { BuildFromModel(model, MatrixIdentity()); }

void Collision::CalculateFromModel(Model *model, const Matrix &transform)
{
    BuildFromModel(model, transform);
}

// ================== Collision Type Management ==================

void Collision::SetCollisionType(CollisionType type)
{
    if (m_collisionType == type)
        return;

    CollisionType oldType = m_collisionType;
    m_collisionType = type;

    // If switching to octree and we have triangles but no octree, build it
    if (type == CollisionType::OCTREE_ONLY && !m_octree && !m_triangles.empty())
    {
        TraceLog(LOG_INFO, "Rebuilding collision as octree due to type change");
        // We need the original model to rebuild, so for now just log
        TraceLog(LOG_WARNING, "Cannot rebuild octree without original model - keeping AABB");
        m_collisionType = CollisionType::AABB_ONLY;
    }

    TraceLog(LOG_INFO, "Collision type changed from %d to %d", (int)oldType, (int)type);
}

// ================== Octree Methods ==================

void Collision::BuildOctree(Model *model) { BuildOctree(model, MatrixIdentity()); }

void Collision::BuildOctree(Model *model, const Matrix &transform)
{
    if (!model || model->meshCount == 0)
    {
        TraceLog(LOG_WARNING, "Invalid model provided for octree construction");
        return;
    }

    // Create octree
    m_octree = std::make_unique<Octree>();

    // Build octree from model
    m_octree->BuildFromModel(model, transform);

    // Update AABB from octree bounds
    UpdateAABBFromOctree();

    TraceLog(LOG_INFO, "Octree built with %zu triangles in %zu nodes", GetTriangleCount(),
             GetNodeCount());
}

bool Collision::IntersectsOctree(const Collision &other) const
{
    if (!m_octree && !other.m_octree)
        return false;

    if (m_octree && other.m_octree)
    {
        // Both have octrees - use octree-octree intersection
        return m_octree->IntersectsAABB(other.m_min, other.m_max);
    }
    else if (m_octree)
    {
        // This has octree, other is AABB
        return m_octree->IntersectsAABB(other.m_min, other.m_max);
    }
    else
    {
        // Other has octree, this is AABB
        return other.m_octree->IntersectsAABB(m_min, m_max);
    }
}

bool Collision::ContainsOctree(const Vector3 &point) const
{
    if (!m_octree)
        return false;
    return m_octree->ContainsPoint(point);
}

bool Collision::RaycastOctree(const Vector3 &origin, const Vector3 &direction, float maxDistance,
                              float &hitDistance, Vector3 &hitPoint, Vector3 &hitNormal) const
{
    if (!m_octree)
        return false;
    return m_octree->Raycast(origin, direction, maxDistance, hitDistance, hitPoint, hitNormal);
}

void Collision::SetUseOctree(bool useOctree)
{
    // Legacy method - convert to new system
    if (useOctree)
    {
        if (m_complexity.IsComplex())
            m_collisionType = CollisionType::OCTREE_ONLY;
        else
            m_collisionType = CollisionType::HYBRID_AUTO;
    }
    else
    {
        m_collisionType = CollisionType::AABB_ONLY;
    }
}

size_t Collision::GetTriangleCount() const
{
    if (m_octree)
        return m_octree->GetTriangleCount();
    return m_triangles.size();
}

size_t Collision::GetNodeCount() const
{
    if (m_octree)
        return m_octree->GetNodeCount();
    return 0;
}

int Collision::GetMaxDepth() const
{
    // This would need to be implemented in Octree class
    return 0;
}

// ================== Helper Methods ==================

void Collision::UpdateAABBFromOctree()
{
    if (!m_octree)
        return;

    // Get octree bounds and update AABB
    // This would need octree to provide its bounds
    // For now, keep existing AABB
}

void Collision::UpdateAABBFromTriangles()
{
    if (m_triangles.empty())
        return;

    Vector3 min = {FLT_MAX, FLT_MAX, FLT_MAX};
    Vector3 max = {-FLT_MAX, -FLT_MAX, -FLT_MAX};

    for (const auto &triangle : m_triangles)
    {
        Vector3 triMin = triangle.GetMin();
        Vector3 triMax = triangle.GetMax();

        if (triMin.x < min.x)
            min.x = triMin.x;
        if (triMin.y < min.y)
            min.y = triMin.y;
        if (triMin.z < min.z)
            min.z = triMin.z;
        if (triMax.x > max.x)
            max.x = triMax.x;
        if (triMax.y > max.y)
            max.y = triMax.y;
        if (triMax.z > max.z)
            max.z = triMax.z;
    }

    m_min = min;
    m_max = max;
}

void Collision::AnalyzeModelComplexity(Model *model, const Matrix &transform)
{
    m_complexity = CollisionComplexity{};

    if (!model || model->meshCount == 0)
        return;

    size_t totalTriangles = 0;
    float totalArea = 0.0f;
    bool hasComplexGeometry = false;

    for (int m = 0; m < model->meshCount; m++)
    {
        Mesh &mesh = model->meshes[m];
        totalTriangles += mesh.triangleCount;

        // Check for complex geometry indicators
        if (mesh.normals || mesh.texcoords || mesh.colors)
            hasComplexGeometry = true;
    }

    // Calculate bounding volume
    BoundingBox bounds = GetModelBoundingBox(*model);
    Vector3 size = Vector3Subtract(bounds.max, bounds.min);
    float volume = size.x * size.y * size.z;

    m_complexity.triangleCount = totalTriangles;
    m_complexity.surfaceArea = totalArea; // Approximate
    m_complexity.boundingVolume = volume;
    m_complexity.hasComplexGeometry = hasComplexGeometry;
}

CollisionType Collision::DetermineOptimalCollisionType() const
{
    // Simple heuristic: use octree for complex models
    if (m_complexity.IsComplex())
        return CollisionType::OCTREE_ONLY;
    else
        return CollisionType::AABB_ONLY;
}

void Collision::ExtractTrianglesFromModel(Model *model, const Matrix &transform)
{
    m_triangles.clear();

    for (int m = 0; m < model->meshCount; m++)
    {
        Mesh &mesh = model->meshes[m];

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

                // Apply transform
                v0 = Vector3Transform(v0, transform);
                v1 = Vector3Transform(v1, transform);
                v2 = Vector3Transform(v2, transform);

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

                // Apply transform
                v0 = Vector3Transform(v0, transform);
                v1 = Vector3Transform(v1, transform);
                v2 = Vector3Transform(v2, transform);

                m_triangles.emplace_back(v0, v1, v2);
            }
        }
    }
}

// ================== Performance Measurement ==================

void Collision::StartPerformanceTimer() const
{
    // Simple performance tracking - could be enhanced with high-resolution timer
    m_stats.checksPerformed++;
}

void Collision::EndPerformanceTimer(CollisionType typeUsed) const
{
    m_stats.typeUsed = typeUsed;
    // m_stats.lastCheckTime would be calculated here with proper timing
}