//
// SmartCollisionSystem.h - Advanced collision system with automatic model subdivision
// Solves the problem of large bounding boxes by intelligently splitting models
//

#ifndef SMART_COLLISION_SYSTEM_H
#define SMART_COLLISION_SYSTEM_H

#include "CollisionStructures.h"
#include "Octree.h"
#include <memory>
#include <raylib.h>
#include <raymath.h>
#include <vector>

// Forward declarations
struct ModelFileConfig;

//
// CollisionSubdivision - represents a subdivided part of a model
//
struct CollisionSubdivision
{
    Vector3 localMin, localMax;               // Local bounding box
    std::vector<CollisionTriangle> triangles; // Triangles in this subdivision
    std::shared_ptr<Octree> octree;           // Octree for this subdivision
    float density;                            // Triangle density (triangles per unit volume)
    bool isActive;                            // Whether this subdivision should be checked

    CollisionSubdivision() : density(0.0f), isActive(true) {}

    // Get world-space bounding box
    Vector3 GetWorldMin(const Matrix &transform) const;
    Vector3 GetWorldMax(const Matrix &transform) const;
    Vector3 GetWorldCenter(const Matrix &transform) const;

    // Check if this subdivision is relevant for collision with given AABB
    bool IsRelevantFor(const Vector3 &testMin, const Vector3 &testMax,
                       const Matrix &transform) const;
};

//
// SmartCollision - Enhanced collision system with automatic subdivision
//
class SmartCollision
{
public:
    // Configuration constants
    static constexpr float MAX_SUBDIVISION_SIZE = 50.0f; // Maximum size before subdivision
    static constexpr float MIN_SUBDIVISION_SIZE = 5.0f;  // Minimum subdivision size
    static constexpr size_t MIN_TRIANGLES_FOR_SUBDIVISION =
        20;                                          // Minimum triangles to justify subdivision
    static constexpr float DENSITY_THRESHOLD = 0.1f; // Minimum density to keep subdivision
    static constexpr size_t MAX_SUBDIVISIONS = 64;   // Maximum number of subdivisions per model

    SmartCollision();
    ~SmartCollision();

    // Copy and move constructors
    SmartCollision(const SmartCollision &other);
    SmartCollision &operator=(const SmartCollision &other);
    SmartCollision(SmartCollision &&other) noexcept;
    SmartCollision &operator=(SmartCollision &&other) noexcept;

    // ================== Main Interface ==================

    // Build smart collision from model
    void BuildFromModel(Model *model, const Matrix &transform = MatrixIdentity());
    void BuildFromModelConfig(Model *model, const ModelFileConfig &config,
                              const Matrix &transform = MatrixIdentity());

    // Update transform (for moving objects)
    void UpdateTransform(const Matrix &newTransform);

    // ================== Collision Queries ==================

    // Check collision with another smart collision system
    bool Intersects(const SmartCollision &other) const;

    // Check collision with simple AABB
    bool IntersectsAABB(const Vector3 &min, const Vector3 &max) const;

    // Check if point is inside
    bool ContainsPoint(const Vector3 &point) const;

    // Ray casting
    bool Raycast(const Vector3 &origin, const Vector3 &direction, float maxDistance,
                 float &hitDistance, Vector3 &hitPoint, Vector3 &hitNormal) const;

    // ================== Getters ==================

    Vector3 GetMin() const { return m_globalMin; }
    Vector3 GetMax() const { return m_globalMax; }
    Vector3 GetCenter() const;
    Vector3 GetSize() const;

    size_t GetSubdivisionCount() const { return m_subdivisions.size(); }
    size_t GetTotalTriangleCount() const;
    size_t GetActiveSubdivisionCount() const;

    // Get subdivision info for debugging
    const std::vector<CollisionSubdivision> &GetSubdivisions() const { return m_subdivisions; }

    // ================== Performance Stats ==================

    struct PerformanceStats
    {
        float lastCheckTime = 0.0f;
        size_t checksPerformed = 0;
        size_t subdivisionsChecked = 0;
        size_t subdivisionsSkipped = 0;
        bool usedEarlyExit = false;
    };

    const PerformanceStats &GetPerformanceStats() const { return m_stats; }

private:
    // Model data
    std::vector<CollisionSubdivision> m_subdivisions;
    Matrix m_transform;
    Vector3 m_globalMin, m_globalMax;

    // Performance tracking
    mutable PerformanceStats m_stats;

    // ================== Internal Methods ==================

    // Model analysis and subdivision
    void AnalyzeAndSubdivideModel(Model *model, const Matrix &transform);
    void CreateSubdivisions(const std::vector<CollisionTriangle> &allTriangles);
    void OptimizeSubdivisions();
    void UpdateGlobalBounds();

    // Subdivision algorithms
    std::vector<CollisionSubdivision>
    SubdivideByDensity(const std::vector<CollisionTriangle> &triangles);
    std::vector<CollisionSubdivision>
    SubdivideBySize(const std::vector<CollisionTriangle> &triangles);
    std::vector<CollisionSubdivision>
    SubdivideAdaptive(const std::vector<CollisionTriangle> &triangles);

    // Helper methods
    void ExtractTrianglesFromModel(Model *model, const Matrix &transform,
                                   std::vector<CollisionTriangle> &triangles);
    float CalculateTriangleDensity(const std::vector<CollisionTriangle> &triangles,
                                   const Vector3 &min, const Vector3 &max);
    bool ShouldSubdivide(const Vector3 &min, const Vector3 &max,
                         const std::vector<CollisionTriangle> &triangles);

    // Performance helpers
    void StartPerformanceTimer() const;
    void EndPerformanceTimer() const;

    // Collision helpers
    bool CheckSubdivisionCollision(const CollisionSubdivision &subdivision, const Vector3 &testMin,
                                   const Vector3 &testMax) const;
    bool CheckSubdivisionPoint(const CollisionSubdivision &subdivision, const Vector3 &point) const;
};

//
// SmartCollisionManager - Enhanced collision manager using smart collision system
//
class SmartCollisionManager
{
public:
    SmartCollisionManager() = default;

    // Add smart collider
    void AddSmartCollider(SmartCollision &&collider);
    void AddSmartCollider(const SmartCollision &collider);

    // Clear all colliders
    void ClearColliders();

    // Check collision with performance optimization
    bool CheckCollision(const SmartCollision &testCollision) const;
    bool CheckCollision(const SmartCollision &testCollision, Vector3 &response) const;

    // Legacy compatibility
    bool CheckCollision(const Vector3 &min, const Vector3 &max) const;
    bool CheckCollision(const Vector3 &min, const Vector3 &max, Vector3 &response) const;

    // Get colliders
    const std::vector<SmartCollision> &GetColliders() const { return m_colliders; }

    // Performance stats
    struct ManagerStats
    {
        size_t totalColliders = 0;
        size_t totalSubdivisions = 0;
        size_t activeSubdivisions = 0;
        float averageCheckTime = 0.0f;
        size_t checksPerformed = 0;
    };

    ManagerStats GetStats() const;

private:
    std::vector<SmartCollision> m_colliders;
    mutable ManagerStats m_stats;

    // Helper methods
    Vector3 CalculateCollisionResponse(const Vector3 &aMin, const Vector3 &aMax,
                                       const Vector3 &bMin, const Vector3 &bMax) const;
};

#endif // SMART_COLLISION_SYSTEM_H
