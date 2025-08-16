//
// SmartCollisionSystem.cpp - Implementation of advanced collision system
//

#include "SmartCollisionSystem.h"
#include "../Model/ModelConfig.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <raylib.h>

// ================== CollisionSubdivision Implementation ==================

Vector3 CollisionSubdivision::GetWorldMin(const Matrix &transform) const
{
    return Vector3Transform(localMin, transform);
}

Vector3 CollisionSubdivision::GetWorldMax(const Matrix &transform) const
{
    return Vector3Transform(localMax, transform);
}

Vector3 CollisionSubdivision::GetWorldCenter(const Matrix &transform) const
{
    Vector3 localCenter = {(localMin.x + localMax.x) * 0.5f, (localMin.y + localMax.y) * 0.5f,
                           (localMin.z + localMax.z) * 0.5f};
    return Vector3Transform(localCenter, transform);
}

bool CollisionSubdivision::IsRelevantFor(const Vector3 &testMin, const Vector3 &testMax,
                                         const Matrix &transform) const
{
    if (!isActive)
        return false;

    Vector3 worldMin = GetWorldMin(transform);
    Vector3 worldMax = GetWorldMax(transform);

    // AABB intersection test
    return (worldMin.x <= testMax.x && worldMax.x >= testMin.x) &&
           (worldMin.y <= testMax.y && worldMax.y >= testMin.y) &&
           (worldMin.z <= testMax.z && worldMax.z >= testMin.z);
}

// ================== SmartCollision Implementation ==================

SmartCollision::SmartCollision() : m_transform(MatrixIdentity())
{
    m_globalMin = {0, 0, 0};
    m_globalMax = {0, 0, 0};
}

SmartCollision::~SmartCollision() = default;

// Copy constructor
SmartCollision::SmartCollision(const SmartCollision &other)
    : m_subdivisions(other.m_subdivisions), m_transform(other.m_transform),
      m_globalMin(other.m_globalMin), m_globalMax(other.m_globalMax)
{
    // Deep copy octrees
    for (auto &subdivision : m_subdivisions)
    {
        if (other.m_subdivisions[&subdivision - &m_subdivisions[0]].octree)
        {
            subdivision.octree = std::make_shared<Octree>();
            // Note: Octree doesn't have copy constructor, so we'll rebuild if needed
            subdivision.octree->Initialize(subdivision.localMin, subdivision.localMax);
            for (const auto &triangle : subdivision.triangles)
            {
                subdivision.octree->AddTriangle(triangle);
            }
        }
    }
}

// Copy assignment
SmartCollision &SmartCollision::operator=(const SmartCollision &other)
{
    if (this != &other)
    {
        m_subdivisions = other.m_subdivisions;
        m_transform = other.m_transform;
        m_globalMin = other.m_globalMin;
        m_globalMax = other.m_globalMax;

        // Deep copy octrees
        for (size_t i = 0; i < m_subdivisions.size(); ++i)
        {
            if (other.m_subdivisions[i].octree)
            {
                m_subdivisions[i].octree = std::make_unique<Octree>();
                m_subdivisions[i].octree->Initialize(m_subdivisions[i].localMin,
                                                     m_subdivisions[i].localMax);
                for (const auto &triangle : m_subdivisions[i].triangles)
                {
                    m_subdivisions[i].octree->AddTriangle(triangle);
                }
            }
        }
    }
    return *this;
}

// Move constructor
SmartCollision::SmartCollision(SmartCollision &&other) noexcept
    : m_subdivisions(std::move(other.m_subdivisions)), m_transform(other.m_transform),
      m_globalMin(other.m_globalMin), m_globalMax(other.m_globalMax)
{
}

// Move assignment
SmartCollision &SmartCollision::operator=(SmartCollision &&other) noexcept
{
    if (this != &other)
    {
        m_subdivisions = std::move(other.m_subdivisions);
        m_transform = other.m_transform;
        m_globalMin = other.m_globalMin;
        m_globalMax = other.m_globalMax;
    }
    return *this;
}

void SmartCollision::BuildFromModel(Model *model, const Matrix &transform)
{
    if (!model || model->meshCount == 0)
    {
        TraceLog(LOG_WARNING, "SmartCollision: Invalid model provided");
        return;
    }

    TraceLog(LOG_INFO, "SmartCollision: Building collision for model with %d meshes",
             model->meshCount);

    m_transform = transform;
    m_subdivisions.clear();

    AnalyzeAndSubdivideModel(model, transform);
    OptimizeSubdivisions();
    UpdateGlobalBounds();

    TraceLog(LOG_INFO,
             "SmartCollision: Created %zu subdivisions (%zu active) with %zu total triangles",
             GetSubdivisionCount(), GetActiveSubdivisionCount(), GetTotalTriangleCount());
}

void SmartCollision::BuildFromModelConfig(Model *model, const ModelFileConfig &config,
                                          const Matrix &transform)
{
    if (!config.hasCollision)
    {
        TraceLog(LOG_INFO, "SmartCollision: Model '%s' has collision disabled",
                 config.name.c_str());
        return;
    }

    TraceLog(LOG_INFO, "SmartCollision: Building collision for model '%s' with precision: %s",
             config.name.c_str(),
             (config.collisionPrecision == CollisionPrecision::AABB_ONLY)       ? "AABB"
             : (config.collisionPrecision == CollisionPrecision::IMPROVED_AABB) ? "IMPROVED"
                                                                                : "PRECISE");

    BuildFromModel(model, transform);
}

void SmartCollision::UpdateTransform(const Matrix &newTransform)
{
    m_transform = newTransform;
    UpdateGlobalBounds();
}

bool SmartCollision::Intersects(const SmartCollision &other) const
{
    StartPerformanceTimer();

    // Early exit: check global bounding boxes first
    if (!((m_globalMin.x <= other.m_globalMax.x && m_globalMax.x >= other.m_globalMin.x) &&
          (m_globalMin.y <= other.m_globalMax.y && m_globalMax.y >= other.m_globalMin.y) &&
          (m_globalMin.z <= other.m_globalMax.z && m_globalMax.z >= other.m_globalMin.z)))
    {
        m_stats.usedEarlyExit = true;
        EndPerformanceTimer();
        return false;
    }

    // Check subdivisions against subdivisions
    for (const auto &mySubdiv : m_subdivisions)
    {
        if (!mySubdiv.isActive)
            continue;

        Vector3 myMin = mySubdiv.GetWorldMin(m_transform);
        Vector3 myMax = mySubdiv.GetWorldMax(m_transform);

        for (const auto &otherSubdiv : other.m_subdivisions)
        {
            if (!otherSubdiv.isActive)
                continue;

            Vector3 otherMin = otherSubdiv.GetWorldMin(other.m_transform);
            Vector3 otherMax = otherSubdiv.GetWorldMax(other.m_transform);

            m_stats.subdivisionsChecked++;

            // AABB intersection test
            if ((myMin.x <= otherMax.x && myMax.x >= otherMin.x) &&
                (myMin.y <= otherMax.y && myMax.y >= otherMin.y) &&
                (myMin.z <= otherMax.z && myMax.z >= otherMin.z))
            {
                // If both have octrees, use precise collision
                if (mySubdiv.octree && otherSubdiv.octree)
                {
                    // For now, return true on AABB intersection
                    // TODO: Implement precise octree-octree collision
                    EndPerformanceTimer();
                    return true;
                }
                else
                {
                    // AABB collision is sufficient
                    EndPerformanceTimer();
                    return true;
                }
            }
        }
    }

    EndPerformanceTimer();
    return false;
}

bool SmartCollision::IntersectsAABB(const Vector3 &min, const Vector3 &max) const
{
    StartPerformanceTimer();

    // Early exit: check global bounding box first
    if (!((m_globalMin.x <= max.x && m_globalMax.x >= min.x) &&
          (m_globalMin.y <= max.y && m_globalMax.y >= min.y) &&
          (m_globalMin.z <= max.z && m_globalMax.z >= min.z)))
    {
        m_stats.usedEarlyExit = true;
        EndPerformanceTimer();
        return false;
    }

    // Check relevant subdivisions
    for (const auto &subdivision : m_subdivisions)
    {
        if (subdivision.IsRelevantFor(min, max, m_transform))
        {
            m_stats.subdivisionsChecked++;

            if (CheckSubdivisionCollision(subdivision, min, max))
            {
                EndPerformanceTimer();
                return true;
            }
        }
        else
        {
            m_stats.subdivisionsSkipped++;
        }
    }

    EndPerformanceTimer();
    return false;
}

bool SmartCollision::ContainsPoint(const Vector3 &point) const
{
    StartPerformanceTimer();

    // Early exit: check global bounding box
    if (point.x < m_globalMin.x || point.x > m_globalMax.x || point.y < m_globalMin.y ||
        point.y > m_globalMax.y || point.z < m_globalMin.z || point.z > m_globalMax.z)
    {
        m_stats.usedEarlyExit = true;
        EndPerformanceTimer();
        return false;
    }

    // Check subdivisions
    for (const auto &subdivision : m_subdivisions)
    {
        if (!subdivision.isActive)
            continue;

        Vector3 worldMin = subdivision.GetWorldMin(m_transform);
        Vector3 worldMax = subdivision.GetWorldMax(m_transform);

        if (point.x >= worldMin.x && point.x <= worldMax.x && point.y >= worldMin.y &&
            point.y <= worldMax.y && point.z >= worldMin.z && point.z <= worldMax.z)
        {
            m_stats.subdivisionsChecked++;

            if (CheckSubdivisionPoint(subdivision, point))
            {
                EndPerformanceTimer();
                return true;
            }
        }
        else
        {
            m_stats.subdivisionsSkipped++;
        }
    }

    EndPerformanceTimer();
    return false;
}

Vector3 SmartCollision::GetCenter() const
{
    return {(m_globalMin.x + m_globalMax.x) * 0.5f, (m_globalMin.y + m_globalMax.y) * 0.5f,
            (m_globalMin.z + m_globalMax.z) * 0.5f};
}

Vector3 SmartCollision::GetSize() const
{
    return {m_globalMax.x - m_globalMin.x, m_globalMax.y - m_globalMin.y,
            m_globalMax.z - m_globalMin.z};
}

size_t SmartCollision::GetTotalTriangleCount() const
{
    size_t total = 0;
    for (const auto &subdivision : m_subdivisions)
    {
        total += subdivision.triangles.size();
    }
    return total;
}

size_t SmartCollision::GetActiveSubdivisionCount() const
{
    return std::count_if(m_subdivisions.begin(), m_subdivisions.end(),
                         [](const CollisionSubdivision &sub) { return sub.isActive; });
}

// ================== Internal Methods ==================

void SmartCollision::AnalyzeAndSubdivideModel(Model *model, const Matrix &transform)
{
    // Extract all triangles from model
    std::vector<CollisionTriangle> allTriangles;
    ExtractTrianglesFromModel(model, transform, allTriangles);

    if (allTriangles.empty())
    {
        TraceLog(LOG_WARNING, "SmartCollision: No triangles extracted from model");
        return;
    }

    TraceLog(LOG_INFO, "SmartCollision: Extracted %zu triangles from model", allTriangles.size());

    // Create subdivisions
    CreateSubdivisions(allTriangles);
}

void SmartCollision::CreateSubdivisions(const std::vector<CollisionTriangle> &allTriangles)
{
    // Calculate overall bounding box
    Vector3 overallMin = {FLT_MAX, FLT_MAX, FLT_MAX};
    Vector3 overallMax = {-FLT_MAX, -FLT_MAX, -FLT_MAX};

    for (const auto &triangle : allTriangles)
    {
        Vector3 triMin = triangle.GetMin();
        Vector3 triMax = triangle.GetMax();

        overallMin.x = fminf(overallMin.x, triMin.x);
        overallMin.y = fminf(overallMin.y, triMin.y);
        overallMin.z = fminf(overallMin.z, triMin.z);

        overallMax.x = fmaxf(overallMax.x, triMax.x);
        overallMax.y = fmaxf(overallMax.y, triMax.y);
        overallMax.z = fmaxf(overallMax.z, triMax.z);
    }

    Vector3 overallSize = {overallMax.x - overallMin.x, overallMax.y - overallMin.y,
                           overallMax.z - overallMin.z};

    TraceLog(LOG_INFO, "SmartCollision: Overall model size: (%.2f, %.2f, %.2f)", overallSize.x,
             overallSize.y, overallSize.z);

    // Decide subdivision strategy
    if (ShouldSubdivide(overallMin, overallMax, allTriangles))
    {
        TraceLog(LOG_INFO, "SmartCollision: Model is large enough for subdivision");
        m_subdivisions = SubdivideAdaptive(allTriangles);
    }
    else
    {
        TraceLog(LOG_INFO, "SmartCollision: Model is small, creating single subdivision");
        // Create single subdivision
        CollisionSubdivision subdivision;
        subdivision.localMin = overallMin;
        subdivision.localMax = overallMax;
        subdivision.triangles = allTriangles;
        subdivision.density = CalculateTriangleDensity(allTriangles, overallMin, overallMax);
        subdivision.isActive = true;

        // Create octree for this subdivision
        subdivision.octree = std::make_unique<Octree>();
        subdivision.octree->Initialize(overallMin, overallMax);
        for (const auto &triangle : allTriangles)
        {
            subdivision.octree->AddTriangle(triangle);
        }

        m_subdivisions.push_back(std::move(subdivision));
    }
}

std::vector<CollisionSubdivision>
SmartCollision::SubdivideAdaptive(const std::vector<CollisionTriangle> &triangles)
{
    std::vector<CollisionSubdivision> subdivisions;

    // Calculate overall bounds
    Vector3 overallMin = {FLT_MAX, FLT_MAX, FLT_MAX};
    Vector3 overallMax = {-FLT_MAX, -FLT_MAX, -FLT_MAX};

    for (const auto &triangle : triangles)
    {
        Vector3 triMin = triangle.GetMin();
        Vector3 triMax = triangle.GetMax();

        overallMin.x = fminf(overallMin.x, triMin.x);
        overallMin.y = fminf(overallMin.y, triMin.y);
        overallMin.z = fminf(overallMin.z, triMin.z);

        overallMax.x = fmaxf(overallMax.x, triMax.x);
        overallMax.y = fmaxf(overallMax.y, triMax.y);
        overallMax.z = fmaxf(overallMax.z, triMax.z);
    }

    Vector3 size = {overallMax.x - overallMin.x, overallMax.y - overallMin.y,
                    overallMax.z - overallMin.z};

    // Determine subdivision count based on size
    int subdivX = static_cast<int>(ceilf(size.x / MAX_SUBDIVISION_SIZE));
    int subdivY = static_cast<int>(ceilf(size.y / MAX_SUBDIVISION_SIZE));
    int subdivZ = static_cast<int>(ceilf(size.z / MAX_SUBDIVISION_SIZE));

    // Limit subdivisions
    subdivX = std::min(subdivX, 8);
    subdivY = std::min(subdivY, 8);
    subdivZ = std::min(subdivZ, 8);

    float stepX = size.x / subdivX;
    float stepY = size.y / subdivY;
    float stepZ = size.z / subdivZ;

    TraceLog(LOG_INFO, "SmartCollision: Creating %dx%dx%d = %d subdivisions", subdivX, subdivY,
             subdivZ, subdivX * subdivY * subdivZ);

    // Create subdivisions
    for (int x = 0; x < subdivX; x++)
    {
        for (int y = 0; y < subdivY; y++)
        {
            for (int z = 0; z < subdivZ; z++)
            {
                Vector3 subMin = {overallMin.x + x * stepX, overallMin.y + y * stepY,
                                  overallMin.z + z * stepZ};

                Vector3 subMax = {overallMin.x + (x + 1) * stepX, overallMin.y + (y + 1) * stepY,
                                  overallMin.z + (z + 1) * stepZ};

                // Find triangles in this subdivision
                std::vector<CollisionTriangle> subTriangles;
                for (const auto &triangle : triangles)
                {
                    if (triangle.IntersectsAABB(subMin, subMax))
                    {
                        subTriangles.push_back(triangle);
                    }
                }

                // Only create subdivision if it has enough triangles
                if (subTriangles.size() >= MIN_TRIANGLES_FOR_SUBDIVISION)
                {
                    CollisionSubdivision subdivision;
                    subdivision.localMin = subMin;
                    subdivision.localMax = subMax;
                    subdivision.triangles = std::move(subTriangles);
                    subdivision.density =
                        CalculateTriangleDensity(subdivision.triangles, subMin, subMax);
                    subdivision.isActive = subdivision.density >= DENSITY_THRESHOLD;

                    // Create octree
                    subdivision.octree = std::make_unique<Octree>();
                    subdivision.octree->Initialize(subMin, subMax);
                    for (const auto &triangle : subdivision.triangles)
                    {
                        subdivision.octree->AddTriangle(triangle);
                    }

                    subdivisions.push_back(std::move(subdivision));
                }
            }
        }
    }

    return subdivisions;
}

void SmartCollision::OptimizeSubdivisions()
{
    // Remove subdivisions with very low density
    size_t originalCount = m_subdivisions.size();

    for (auto &subdivision : m_subdivisions)
    {
        if (subdivision.density < DENSITY_THRESHOLD)
        {
            subdivision.isActive = false;
        }
    }

    size_t activeCount = GetActiveSubdivisionCount();
    TraceLog(LOG_INFO, "SmartCollision: Optimized subdivisions: %zu -> %zu active", originalCount,
             activeCount);
}

void SmartCollision::UpdateGlobalBounds()
{
    if (m_subdivisions.empty())
    {
        m_globalMin = m_globalMax = {0, 0, 0};
        return;
    }

    m_globalMin = {FLT_MAX, FLT_MAX, FLT_MAX};
    m_globalMax = {-FLT_MAX, -FLT_MAX, -FLT_MAX};

    for (const auto &subdivision : m_subdivisions)
    {
        if (!subdivision.isActive)
            continue;

        Vector3 worldMin = subdivision.GetWorldMin(m_transform);
        Vector3 worldMax = subdivision.GetWorldMax(m_transform);

        m_globalMin.x = fminf(m_globalMin.x, worldMin.x);
        m_globalMin.y = fminf(m_globalMin.y, worldMin.y);
        m_globalMin.z = fminf(m_globalMin.z, worldMin.z);

        m_globalMax.x = fmaxf(m_globalMax.x, worldMax.x);
        m_globalMax.y = fmaxf(m_globalMax.y, worldMax.y);
        m_globalMax.z = fmaxf(m_globalMax.z, worldMax.z);
    }
}

// Helper methods implementation continues...
// (Due to length limits, I'll continue with the most important parts)

void SmartCollision::ExtractTrianglesFromModel(Model *model, const Matrix &transform,
                                               std::vector<CollisionTriangle> &triangles)
{
    for (int meshIndex = 0; meshIndex < model->meshCount; meshIndex++)
    {
        Mesh &mesh = model->meshes[meshIndex];

        if (mesh.vertices == nullptr || mesh.triangleCount == 0)
            continue;

        for (int i = 0; i < mesh.triangleCount; i++)
        {
            int idx0, idx1, idx2;

            if (mesh.indices)
            {
                idx0 = mesh.indices[i * 3 + 0];
                idx1 = mesh.indices[i * 3 + 1];
                idx2 = mesh.indices[i * 3 + 2];
            }
            else
            {
                idx0 = i * 3 + 0;
                idx1 = i * 3 + 1;
                idx2 = i * 3 + 2;
            }

            Vector3 v0 = {mesh.vertices[idx0 * 3 + 0], mesh.vertices[idx0 * 3 + 1],
                          mesh.vertices[idx0 * 3 + 2]};
            Vector3 v1 = {mesh.vertices[idx1 * 3 + 0], mesh.vertices[idx1 * 3 + 1],
                          mesh.vertices[idx1 * 3 + 2]};
            Vector3 v2 = {mesh.vertices[idx2 * 3 + 0], mesh.vertices[idx2 * 3 + 1],
                          mesh.vertices[idx2 * 3 + 2]};

            // Transform vertices
            v0 = Vector3Transform(v0, transform);
            v1 = Vector3Transform(v1, transform);
            v2 = Vector3Transform(v2, transform);

            triangles.emplace_back(v0, v1, v2);
        }
    }
}

float SmartCollision::CalculateTriangleDensity(const std::vector<CollisionTriangle> &triangles,
                                               const Vector3 &min, const Vector3 &max)
{
    if (triangles.empty())
        return 0.0f;

    float volume = (max.x - min.x) * (max.y - min.y) * (max.z - min.z);
    if (volume <= 0.0f)
        return 0.0f;

    return static_cast<float>(triangles.size()) / volume;
}

bool SmartCollision::ShouldSubdivide(const Vector3 &min, const Vector3 &max,
                                     const std::vector<CollisionTriangle> &triangles)
{
    Vector3 size = {max.x - min.x, max.y - min.y, max.z - min.z};

    // Check if any dimension is larger than threshold
    bool isTooLarge = (size.x > MAX_SUBDIVISION_SIZE) || (size.y > MAX_SUBDIVISION_SIZE) ||
                      (size.z > MAX_SUBDIVISION_SIZE);

    // Check if we have enough triangles to justify subdivision
    bool hasEnoughTriangles = triangles.size() >= MIN_TRIANGLES_FOR_SUBDIVISION * 4;

    return isTooLarge && hasEnoughTriangles;
}

bool SmartCollision::CheckSubdivisionCollision(const CollisionSubdivision &subdivision,
                                               const Vector3 &testMin, const Vector3 &testMax) const
{
    if (subdivision.octree)
    {
        return subdivision.octree->IntersectsAABB(testMin, testMax);
    }
    else
    {
        // Fallback to AABB
        Vector3 worldMin = subdivision.GetWorldMin(m_transform);
        Vector3 worldMax = subdivision.GetWorldMax(m_transform);

        return (worldMin.x <= testMax.x && worldMax.x >= testMin.x) &&
               (worldMin.y <= testMax.y && worldMax.y >= testMin.y) &&
               (worldMin.z <= testMax.z && worldMax.z >= testMin.z);
    }
}

bool SmartCollision::CheckSubdivisionPoint(const CollisionSubdivision &subdivision,
                                           const Vector3 &point) const
{
    if (subdivision.octree)
    {
        return subdivision.octree->ContainsPoint(point);
    }
    else
    {
        // Fallback to AABB
        Vector3 worldMin = subdivision.GetWorldMin(m_transform);
        Vector3 worldMax = subdivision.GetWorldMax(m_transform);

        return (point.x >= worldMin.x && point.x <= worldMax.x) &&
               (point.y >= worldMin.y && point.y <= worldMax.y) &&
               (point.z >= worldMin.z && point.z <= worldMax.z);
    }
}

void SmartCollision::StartPerformanceTimer() const
{
    // Implementation for performance timing
}

void SmartCollision::EndPerformanceTimer() const { m_stats.checksPerformed++; }

// ================== SmartCollisionManager Implementation ==================

void SmartCollisionManager::AddSmartCollider(SmartCollision &&collider)
{
    m_colliders.push_back(std::move(collider));
    m_stats.totalColliders++;
}

void SmartCollisionManager::AddSmartCollider(const SmartCollision &collider)
{
    m_colliders.push_back(collider);
    m_stats.totalColliders++;
}

void SmartCollisionManager::ClearColliders()
{
    m_colliders.clear();
    m_stats = ManagerStats{};
}

bool SmartCollisionManager::CheckCollision(const SmartCollision &testCollision) const
{
    for (const auto &collider : m_colliders)
    {
        if (testCollision.Intersects(collider))
        {
            return true;
        }
    }
    return false;
}

bool SmartCollisionManager::CheckCollision(const Vector3 &min, const Vector3 &max) const
{
    for (const auto &collider : m_colliders)
    {
        if (collider.IntersectsAABB(min, max))
        {
            return true;
        }
    }
    return false;
}

bool SmartCollisionManager::CheckCollision(const Vector3 &min, const Vector3 &max,
                                           Vector3 &response) const
{
    for (const auto &collider : m_colliders)
    {
        if (collider.IntersectsAABB(min, max))
        {
            // Calculate collision response using collider bounds
            Vector3 colliderMin = collider.GetMin();
            Vector3 colliderMax = collider.GetMax();

            response = CalculateCollisionResponse(min, max, colliderMin, colliderMax);
            return true;
        }
    }
    return false;
}

SmartCollisionManager::ManagerStats SmartCollisionManager::GetStats() const
{
    ManagerStats stats = m_stats;

    for (const auto &collider : m_colliders)
    {
        stats.totalSubdivisions += collider.GetSubdivisionCount();
        stats.activeSubdivisions += collider.GetActiveSubdivisionCount();
    }

    return stats;
}

Vector3 SmartCollisionManager::CalculateCollisionResponse(const Vector3 &aMin, const Vector3 &aMax,
                                                          const Vector3 &bMin,
                                                          const Vector3 &bMax) const
{
    // Calculate Minimum Translation Vector (MTV) for collision response
    float dx1 = bMax.x - aMin.x;
    float dx2 = aMax.x - bMin.x;
    float dy1 = bMax.y - aMin.y;
    float dy2 = aMax.y - bMin.y;
    float dz1 = bMax.z - aMin.z;
    float dz2 = aMax.z - bMin.z;

    float dx = (dx1 < dx2) ? dx1 : -dx2;
    float dy = (dy1 < dy2) ? dy1 : -dy2;
    float dz = (dz1 < dz2) ? dz1 : -dz2;

    float absDx = fabsf(dx);
    float absDy = fabsf(dy);
    float absDz = fabsf(dz);

    // Return the smallest displacement vector
    if (absDx < absDy && absDx < absDz)
        return {dx, 0, 0};
    else if (absDy < absDz)
        return {0, dy, 0};
    else
        return {0, 0, dz};
}