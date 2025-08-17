//
// CollisionSystem.cpp - Hybrid AABB + Octree Implementation
// Created by Assistant for optimized collision detection
//

#include "CollisionSystem.h"
#include "../Model/ModelConfig.h"
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

    // Determine which collision method to use first
    CollisionType thisType = (m_collisionType == CollisionType::HYBRID_AUTO)
                                 ? DetermineOptimalCollisionType()
                                 : m_collisionType;
    CollisionType otherType = (other.m_collisionType == CollisionType::HYBRID_AUTO)
                                  ? other.DetermineOptimalCollisionType()
                                  : other.m_collisionType;

    // Choose collision method based on precision requirements
    CollisionType finalType = (thisType > otherType) ? thisType : otherType; // Use higher precision

    // For precise collision types, we may skip or modify AABB broad-phase check
    bool skipAABBCheck = (finalType == CollisionType::TRIANGLE_PRECISE);

    if (!skipAABBCheck)
    {
        // AABB broad-phase check for non-precise collision types
        bool aabbIntersects = (m_min.x <= other.m_max.x && m_max.x >= other.m_min.x) &&
                              (m_min.y <= other.m_max.y && m_max.y >= other.m_min.y) &&
                              (m_min.z <= other.m_max.z && m_max.z >= other.m_min.z);

        if (!aabbIntersects)
        {
            EndPerformanceTimer(CollisionType::AABB_ONLY);
            return false;
        }
    }

    switch (finalType)
    {
    case CollisionType::AABB_ONLY:
        EndPerformanceTimer(CollisionType::AABB_ONLY);
        return true; // AABB already passed

    case CollisionType::IMPROVED_AABB:
        if (m_octree && other.m_octree)
        {
            bool result = m_octree->IntersectsImproved(other.m_min, other.m_max) ||
                          other.m_octree->IntersectsImproved(m_min, m_max);
            EndPerformanceTimer(CollisionType::IMPROVED_AABB);
            return result;
        }
        else if (m_octree)
        {
            bool result = m_octree->IntersectsImproved(other.m_min, other.m_max);
            EndPerformanceTimer(CollisionType::IMPROVED_AABB);
            return result;
        }
        else if (other.m_octree)
        {
            bool result = other.m_octree->IntersectsImproved(m_min, m_max);
            EndPerformanceTimer(CollisionType::IMPROVED_AABB);
            return result;
        }
        // Fallback to AABB
        EndPerformanceTimer(CollisionType::AABB_ONLY);
        return true;

    case CollisionType::TRIANGLE_PRECISE:
        // For precise collision, we need to check if the other object (usually player)
        // intersects with our triangle mesh
        if (m_octree)
        {
            // Check if any corner of the other object's AABB is inside our mesh
            Vector3 corners[8] = {{other.m_min.x, other.m_min.y, other.m_min.z},
                                  {other.m_max.x, other.m_min.y, other.m_min.z},
                                  {other.m_min.x, other.m_max.y, other.m_min.z},
                                  {other.m_max.x, other.m_max.y, other.m_min.z},
                                  {other.m_min.x, other.m_min.y, other.m_max.z},
                                  {other.m_max.x, other.m_min.y, other.m_max.z},
                                  {other.m_min.x, other.m_max.y, other.m_max.z},
                                  {other.m_max.x, other.m_max.y, other.m_max.z}};

            for (int i = 0; i < 8; i++)
            {
                if (m_octree->ContainsPoint(corners[i]))
                {
                    TraceLog(LOG_DEBUG,
                             "PRECISE COLLISION: Corner %d (%.2f, %.2f, %.2f) intersects mesh", i,
                             corners[i].x, corners[i].y, corners[i].z);
                    EndPerformanceTimer(CollisionType::TRIANGLE_PRECISE);
                    return true;
                }
            }

            // Also check center point
            Vector3 center = {(other.m_min.x + other.m_max.x) * 0.5f,
                              (other.m_min.y + other.m_max.y) * 0.5f,
                              (other.m_min.z + other.m_max.z) * 0.5f};

            if (m_octree->ContainsPoint(center))
            {
                TraceLog(LOG_DEBUG, "PRECISE COLLISION: Center (%.2f, %.2f, %.2f) intersects mesh",
                         center.x, center.y, center.z);
                EndPerformanceTimer(CollisionType::TRIANGLE_PRECISE);
                return true;
            }
        }

        // If other object has octree, check the reverse
        if (other.m_octree)
        {
            Vector3 corners[8] = {{m_min.x, m_min.y, m_min.z}, {m_max.x, m_min.y, m_min.z},
                                  {m_min.x, m_max.y, m_min.z}, {m_max.x, m_max.y, m_min.z},
                                  {m_min.x, m_min.y, m_max.z}, {m_max.x, m_min.y, m_max.z},
                                  {m_min.x, m_max.y, m_max.z}, {m_max.x, m_max.y, m_max.z}};

            for (int i = 0; i < 8; i++)
            {
                if (other.m_octree->ContainsPoint(corners[i]))
                {
                    EndPerformanceTimer(CollisionType::TRIANGLE_PRECISE);
                    return true;
                }
            }
        }

        EndPerformanceTimer(CollisionType::TRIANGLE_PRECISE);
        return false;

    case CollisionType::OCTREE_ONLY:
    default:
        bool result = IntersectsOctree(other);
        EndPerformanceTimer(CollisionType::OCTREE_ONLY);
        return result;
    }
}

bool Collision::Contains(const Vector3 &point) const
{
    StartPerformanceTimer();

    // Determine collision method first
    CollisionType type = (m_collisionType == CollisionType::HYBRID_AUTO)
                             ? DetermineOptimalCollisionType()
                             : m_collisionType;

    // For precise collision types, skip AABB check and go directly to octree
    if (type == CollisionType::TRIANGLE_PRECISE && m_octree)
    {
        bool result = ContainsOctree(point);
        // Debug logging for precise collision
        if (result)
        {
            TraceLog(LOG_DEBUG,
                     "PRECISE COLLISION: Point (%.2f, %.2f, %.2f) intersects with surface", point.x,
                     point.y, point.z);
        }
        EndPerformanceTimer(CollisionType::TRIANGLE_PRECISE);
        return result;
    }

    // AABB check for other types
    bool aabbContains = (point.x >= m_min.x && point.x <= m_max.x) &&
                        (point.y >= m_min.y && point.y <= m_max.y) &&
                        (point.z >= m_min.z && point.z <= m_max.z);

    if (!aabbContains)
    {
        EndPerformanceTimer(CollisionType::AABB_ONLY);
        return false;
    }

    // Handle other collision types
    switch (type)
    {
    case CollisionType::IMPROVED_AABB:
        if (m_octree)
        {
            bool result = ContainsOctree(point);
            EndPerformanceTimer(CollisionType::IMPROVED_AABB);
            return result;
        }
        break;

    case CollisionType::OCTREE_ONLY:
        if (m_octree)
        {
            bool result = ContainsOctree(point);
            EndPerformanceTimer(CollisionType::OCTREE_ONLY);
            return result;
        }
        break;

    case CollisionType::AABB_ONLY:
    default:
        break;
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

    // Simple model hash to prevent duplicate processing
    size_t modelHash = reinterpret_cast<size_t>(model) ^ static_cast<size_t>(type) ^
                       static_cast<size_t>(model->meshCount);

    if (m_isBuilt && m_modelHash == modelHash)
    {
        TraceLog(LOG_INFO, "Collision already built for this model configuration, skipping...");
        return;
    }

    m_modelHash = modelHash;
    m_isBuilt = true;

    // Force specific collision type
    m_collisionType = type;

    // Analyze model complexity anyway for stats
    AnalyzeModelComplexity(model, transform);
    ExtractTrianglesFromModel(model, transform);
    UpdateAABBFromTriangles();

    // Build appropriate collision structure based on type
    switch (type)
    {
    case CollisionType::AABB_ONLY:
        TraceLog(LOG_INFO, "Using AABB-only collision");
        break;

    case CollisionType::IMPROVED_AABB:
        BuildOctree(model, transform);
        TraceLog(LOG_INFO, "Built improved AABB collision with %zu nodes", GetNodeCount());
        break;

    case CollisionType::TRIANGLE_PRECISE:
        BuildOctree(model, transform);
        TraceLog(LOG_INFO, "Built precise triangle collision with %zu nodes and %zu triangles",
                 GetNodeCount(), GetTriangleCount());
        break;

    case CollisionType::OCTREE_ONLY:
    default:
        BuildOctree(model, transform);
        TraceLog(LOG_INFO, "Built octree collision with %zu nodes", GetNodeCount());
        break;
    }
}

void Collision::BuildFromModelConfig(Model *model, const ModelFileConfig &config,
                                     const Matrix &transform)
{
    if (!model || model->meshCount == 0)
    {
        TraceLog(LOG_WARNING, "Invalid model provided for collision building");
        return;
    }

    // Convert CollisionPrecision to CollisionType
    CollisionType targetType;
    switch (config.collisionPrecision)
    {
    case CollisionPrecision::AUTO:
        // First analyze model complexity, then determine optimal type
        AnalyzeModelComplexity(model, transform);
        ExtractTrianglesFromModel(model, transform);
        targetType = DetermineOptimalCollisionType();
        TraceLog(LOG_INFO, "AUTO collision type selected: %s for model '%s'",
                 (targetType == CollisionType::AABB_ONLY)       ? "AABB"
                 : (targetType == CollisionType::IMPROVED_AABB) ? "IMPROVED"
                                                                : "PRECISE",
                 config.name.c_str());
        break;
    case CollisionPrecision::AABB_ONLY:
        targetType = CollisionType::AABB_ONLY;
        break;
    case CollisionPrecision::IMPROVED_AABB:
        targetType = CollisionType::IMPROVED_AABB;
        break;
    case CollisionPrecision::TRIANGLE_PRECISE:
        targetType = CollisionType::TRIANGLE_PRECISE;
        break;
    default:
        targetType = CollisionType::AABB_ONLY; // Safest default
        break;
    }

    TraceLog(LOG_INFO, "Building collision for model '%s' with precision: %s", config.name.c_str(),
             (targetType == CollisionType::AABB_ONLY)       ? "AABB"
             : (targetType == CollisionType::IMPROVED_AABB) ? "IMPROVED"
                                                            : "PRECISE");

    // Use the specific type method
    BuildFromModel(model, targetType, transform);
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
    // Enhanced heuristic with safety limits to prevent memory issues
    size_t triangleCount = m_complexity.triangleCount;

    // Safety: Never use precise collision for models with many triangles
    if (triangleCount > 1000)
    {
        TraceLog(LOG_INFO, "Model has %zu triangles - using AABB for performance", triangleCount);
        return CollisionType::AABB_ONLY;
    }
    else if (triangleCount > 500)
    {
        TraceLog(LOG_INFO, "Model has %zu triangles - using improved AABB", triangleCount);
        return CollisionType::IMPROVED_AABB;
    }
    else if (triangleCount > 100)
    {
        TraceLog(LOG_INFO, "Model has %zu triangles - using improved collision", triangleCount);
        return CollisionType::IMPROVED_AABB;
    }
    else
    {
        TraceLog(LOG_INFO, "Model has %zu triangles - using AABB only", triangleCount);
        return CollisionType::AABB_ONLY; // Changed: Use AABB for small models too - safer
    }
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
            for (int i = 0; i < mesh.triangleCount; i++)
            {
                unsigned short i0 = mesh.indices[i * 3 + 0];
                unsigned short i1 = mesh.indices[i * 3 + 1];
                unsigned short i2 = mesh.indices[i * 3 + 2];

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