//
// Created by I#Oleg
// Collision System - Hybrid AABB + Octree Implementation
// Automatically chooses optimal collision method based on model complexity
//

#ifndef COLLISIONSYSTEM_H
#define COLLISIONSYSTEM_H

#include "CollisionStructures.h"
#include <cfloat>
#include <memory>
#include <raylib.h>
#include <raymath.h>
#include <vector>

// Forward declaration
struct ModelFileConfig;

// Forward declaration for Octree
class Octree;

//
// Collision
// Hybrid collision detection system that automatically chooses between:
// - AABB: Fast collision for simple models (< 100 triangles)
// - Octree: Precise collision for complex models (>= 100 triangles)
// - Manual override available for specific use cases
//
class Collision
{
public:
    Collision();

    // Initialize collision box by center position and size (half-extents)
    Collision(const Vector3 &center, const Vector3 &size);

    // Copy constructor and assignment operator (needed due to std::unique_ptr)
    Collision(const Collision &other);
    Collision &operator=(const Collision &other);

    // Move constructor and assignment operator
    Collision(Collision &&other) noexcept;
    Collision &operator=(Collision &&other) noexcept;

    // Custom destructor (needed for unique_ptr<Octree> with forward declaration)
    ~Collision();

    // -------------------- Getters --------------------

    // Get minimum corner of bounding box
    Vector3 GetMin() const;

    // Get maximum corner of bounding box
    Vector3 GetMax() const;

    // Get center of bounding box
    Vector3 GetCenter() const;

    // Get size of bounding box
    Vector3 GetSize() const;

    // -------------------- Update --------------------

    // Update bounding box position and size
    void Update(const Vector3 &center, const Vector3 &size);

    // -------------------- AABB Collision Checks --------------------

    // Check if this collision box intersects with another (AABB)
    bool Intersects(const Collision &other) const;

    // Check if this collision box contains a point (AABB)
    bool Contains(const Vector3 &point) const;

    // -------------------- Hybrid Model Collision --------------------

    // Build collision from model with automatic complexity detection
    void BuildFromModel(Model *model, const Matrix &transform = MatrixIdentity());

    // Build collision with specific type (override automatic detection)
    void BuildFromModel(Model *model, CollisionType type,
                        const Matrix &transform = MatrixIdentity());

    // Build collision from model config (uses precision setting from config)
    void BuildFromModelConfig(Model *model, const struct ModelFileConfig &config,
                              const Matrix &transform = MatrixIdentity());

    // Legacy methods (for backward compatibility)
    void CalculateFromModel(Model *model);
    void CalculateFromModel(Model *model, const Matrix &transform);

    // -------------------- Collision Type Management --------------------

    // Get current collision type
    CollisionType GetCollisionType() const { return m_collisionType; }

    // Set collision type (will rebuild if necessary)
    void SetCollisionType(CollisionType type);

    // Get model complexity analysis
    const CollisionComplexity &GetComplexity() const { return m_complexity; }

    // -------------------- Octree Methods --------------------

    // Build Octree from model for precise collision detection
    void BuildOctree(Model *model);
    void BuildOctree(Model *model, const Matrix &transform);

    // Check collision using Octree (more precise than AABB)
    bool IntersectsOctree(const Collision &other) const;

    // Point-in-mesh test using Octree
    bool ContainsOctree(const Vector3 &point) const;

    // Ray casting with Octree
    bool RaycastOctree(const Vector3 &origin, const Vector3 &direction, float maxDistance,
                       float &hitDistance, Vector3 &hitPoint, Vector3 &hitNormal) const;

    // Enable/disable Octree usage (legacy)
    void SetUseOctree(bool useOctree);
    bool IsUsingOctree() const
    {
        return m_collisionType == CollisionType::OCTREE_ONLY && m_octree != nullptr;
    }
    
    // Force initialization of octree if needed
    void InitializeOctree()
    {
        if ((m_collisionType == CollisionType::OCTREE_ONLY || 
             m_collisionType == CollisionType::TRIANGLE_PRECISE ||
             m_collisionType == CollisionType::IMPROVED_AABB) && 
            !m_triangles.empty()) {
            EnsureOctree();
        }
    }

    // Get triangle count
    size_t GetTriangleCount() const;

    // Get debug information
    size_t GetNodeCount() const;
    int GetMaxDepth() const;

    // -------------------- Triangle Access --------------------
    Octree* GetOctree();

    // -------------------- Performance Methods --------------------

    // Get collision detection performance stats
    struct PerformanceStats
    {
        float lastCheckTime = 0.0f;
        size_t checksPerformed = 0;
        CollisionType typeUsed = CollisionType::AABB_ONLY;
    };

    const PerformanceStats &GetPerformanceStats() const { return m_stats; }

private:
    // AABB data (always maintained for broad-phase)
    Vector3 m_min{}; // Minimum corner of AABB
    Vector3 m_max{}; // Maximum corner of AABB

    // Collision system data
    CollisionType m_collisionType = CollisionType::HYBRID_AUTO;
    CollisionComplexity m_complexity;
    std::vector<CollisionTriangle> m_triangles;

    // Octree data for precise collision detection
    mutable std::unique_ptr<Octree> m_octree;

    // Caching to prevent duplicate processing
    size_t m_modelHash = 0;
    bool m_isBuilt = false;

    // Performance tracking
    mutable PerformanceStats m_stats;

    // Helper methods
    void UpdateAABBFromOctree();
    void UpdateAABBFromTriangles();
    void AnalyzeModelComplexity(Model *model, const Matrix &transform);
    void EnsureOctree() const; // Build octree from triangles if needed
    CollisionType DetermineOptimalCollisionType() const;
    void ExtractTrianglesFromModel(Model *model, const Matrix &transform);

    // Performance measurement helpers
    void StartPerformanceTimer() const;
    void EndPerformanceTimer(CollisionType typeUsed) const;
};

#endif // COLLISIONSYSTEM_H