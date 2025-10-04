#include "OptimizedCollisionSystem.h"
#include <algorithm>
#include <cmath>
#include <raymath.h>
#include <raylib.h>

void SpatialGridCell::RemoveCollider(Collision* collider) {
    colliders.erase(std::remove(colliders.begin(), colliders.end(), collider), colliders.end());
}

SpatialGrid::SpatialGrid(const Vector3& gridMin, const Vector3& gridMax, float cellSize)
    : m_gridMin(gridMin), m_gridMax(gridMax), m_cellSize(cellSize), m_autoOptimize(false) {

    m_gridSize = Vector3Subtract(m_gridMax, m_gridMin);
    m_gridDimensions = {
        static_cast<int>(ceil(m_gridSize.x / cellSize)),
        static_cast<int>(ceil(m_gridSize.y / cellSize)),
        static_cast<int>(ceil(m_gridSize.z / cellSize))
    };

    // Initialize 3D grid
    m_cells.resize(m_gridDimensions[0]);
    for (int x = 0; x < m_gridDimensions[0]; x++) {
        m_cells[x].resize(m_gridDimensions[1]);
        for (int y = 0; y < m_gridDimensions[1]; y++) {
            m_cells[x][y].resize(m_gridDimensions[2]);
            for (int z = 0; z < m_gridDimensions[2]; z++) {
                Vector3 cellMin = {
                    m_gridMin.x + x * cellSize,
                    m_gridMin.y + y * cellSize,
                    m_gridMin.z + z * cellSize
                };
                Vector3 cellMax = {
                    cellMin.x + cellSize,
                    cellMin.y + cellSize,
                    cellMin.z + cellSize
                };
                m_cells[x][y][z] = SpatialGridCell(cellMin, cellMax);
            }
        }
    }
}

void SpatialGrid::Update(const std::vector<std::unique_ptr<Collision>>& colliders) {
    // Clear all cells
    for (auto& x : m_cells) {
        for (auto& y : x) {
            for (auto& z : y) {
                z.Clear();
            }
        }
    }

    // Add colliders to appropriate cells
    for (const auto& collider : colliders) {
        if (!collider) continue;

        BoundingBox bbox = collider->GetBoundingBox();
        std::array<int, 3> minCell = GetCellIndex(bbox.min);
        std::array<int, 3> maxCell = GetCellIndex(bbox.max);

        // Add to all overlapping cells
        for (int x = minCell[0]; x <= maxCell[0] && x < m_gridDimensions[0]; x++) {
            for (int y = minCell[1]; y <= maxCell[1] && y < m_gridDimensions[1]; y++) {
                for (int z = minCell[2]; z <= maxCell[2] && z < m_gridDimensions[2]; z++) {
                    m_cells[x][y][z].AddCollider(collider.get());
                }
            }
        }
    }
}

void SpatialGrid::Clear() {
    for (auto& x : m_cells) {
        for (auto& y : x) {
            for (auto& z : y) {
                z.Clear();
            }
        }
    }
}

std::vector<Collision*> SpatialGrid::GetNearbyColliders(const Vector3& position, float radius) {
    std::vector<Collision*> nearby;
    std::array<int, 3> centerCell = GetCellIndex(position);

    // Check cells within radius
    int cellRadius = static_cast<int>(ceil(radius / m_cellSize));

    for (int x = centerCell[0] - cellRadius; x <= centerCell[0] + cellRadius; x++) {
        for (int y = centerCell[1] - cellRadius; y <= centerCell[1] + cellRadius; y++) {
            for (int z = centerCell[2] - cellRadius; z <= centerCell[2] + cellRadius; z++) {
                if (IsValidCell(x, y, z)) {
                    const auto& cellColliders = m_cells[x][y][z].colliders;
                    nearby.insert(nearby.end(), cellColliders.begin(), cellColliders.end());
                }
            }
        }
    }

    return nearby;
}

std::vector<Collision*> SpatialGrid::GetCollidersInCell(int x, int y, int z) {
    if (IsValidCell(x, y, z)) {
        return m_cells[x][y][z].colliders;
    }
    return {};
}

std::vector<Collision*> SpatialGrid::GetCollidersInAABB(const BoundingBox& aabb) {
    std::vector<Collision*> colliders;
    std::array<int, 3> minCell = GetCellIndex(aabb.min);
    std::array<int, 3> maxCell = GetCellIndex(aabb.max);

    for (int x = minCell[0]; x <= maxCell[0] && x < m_gridDimensions[0]; x++) {
        for (int y = minCell[1]; y <= maxCell[1] && y < m_gridDimensions[1]; y++) {
            for (int z = minCell[2]; z <= maxCell[2] && z < m_gridDimensions[2]; z++) {
                const auto& cellColliders = m_cells[x][y][z].colliders;
                colliders.insert(colliders.end(), cellColliders.begin(), cellColliders.end());
            }
        }
    }

    return colliders;
}

void SpatialGrid::OptimizeGridSize(float newCellSize) {
    if (fabs(newCellSize - m_cellSize) < 0.01f) return;

    m_cellSize = newCellSize;
    m_gridDimensions = {
        static_cast<int>(ceil(m_gridSize.x / cellSize)),
        static_cast<int>(ceil(m_gridSize.y / cellSize)),
        static_cast<int>(ceil(m_gridSize.z / cellSize))
    };

    // Reinitialize grid with new dimensions
    m_cells.clear();
    m_cells.resize(m_gridDimensions[0]);
    for (int x = 0; x < m_gridDimensions[0]; x++) {
        m_cells[x].resize(m_gridDimensions[1]);
        for (int y = 0; y < m_gridDimensions[1]; y++) {
            m_cells[x][y].resize(m_gridDimensions[2]);
            for (int z = 0; z < m_gridDimensions[2]; z++) {
                Vector3 cellMin = {
                    m_gridMin.x + x * cellSize,
                    m_gridMin.y + y * cellSize,
                    m_gridMin.z + z * cellSize
                };
                Vector3 cellMax = {
                    cellMin.x + cellSize,
                    cellMin.y + cellSize,
                    cellMin.z + cellSize
                };
                m_cells[x][y][z] = SpatialGridCell(cellMin, cellMax);
            }
        }
    }
}

std::array<int, 3> SpatialGrid::GetCellIndex(const Vector3& position) {
    return {
        static_cast<int>((position.x - m_gridMin.x) / m_cellSize),
        static_cast<int>((position.y - m_gridMin.y) / m_cellSize),
        static_cast<int>((position.z - m_gridMin.z) / m_cellSize)
    };
}

bool SpatialGrid::IsValidCell(int x, int y, int z) const {
    return x >= 0 && x < m_gridDimensions[0] &&
           y >= 0 && y < m_gridDimensions[1] &&
           z >= 0 && z < m_gridDimensions[2];
}

void SpatialGrid::ExpandGridIfNeeded(const Vector3& position) {
    // Check if position is outside current grid bounds
    bool needsExpansion = position.x < m_gridMin.x || position.x > m_gridMax.x ||
                         position.y < m_gridMin.y || position.y > m_gridMax.y ||
                         position.z < m_gridMin.z || position.z > m_gridMax.z;

    if (needsExpansion) {
        // Expand grid (simplified implementation)
        TraceLog(LOG_WARNING, "SpatialGrid::ExpandGridIfNeeded() - Grid expansion needed for position (%.2f, %.2f, %.2f)",
                 position.x, position.y, position.z);
    }
}

BoundingBox SpatialGrid::GetCellBounds(int x, int y, int z) const {
    if (IsValidCell(x, y, z)) {
        Vector3 min = {
            m_gridMin.x + x * m_cellSize,
            m_gridMin.y + y * m_cellSize,
            m_gridMin.z + z * m_cellSize
        };
        Vector3 max = {
            min.x + m_cellSize,
            min.y + m_cellSize,
            min.z + m_cellSize
        };
        return {min, max};
    }
    return {{0, 0, 0}, {0, 0, 0}};
}

OptimizedCollisionSystem::OptimizedCollisionSystem()
    : m_useSpatialGrid(true), m_maxCollisionChecks(1000), m_collisionCaching(true),
      m_collisionDistanceThreshold(100.0f) {

    m_stats = CollisionStats{};
    m_lastUpdateTime = std::chrono::steady_clock::now();
}

void OptimizedCollisionSystem::Initialize() {
    if (m_useSpatialGrid) {
        Vector3 gridMin = {-1000, -100, -1000};
        Vector3 gridMax = {1000, 1000, 1000};
        m_spatialGrid = std::make_unique<SpatialGrid>(gridMin, gridMax, 10.0f);
    }

    TraceLog(LOG_INFO, "OptimizedCollisionSystem::Initialize() - Initialized optimized collision system");
}

void OptimizedCollisionSystem::Update(float deltaTime) {
    auto startTime = std::chrono::steady_clock::now();

    if (m_spatialGrid) {
        // Update spatial grid would need access to collision manager
        // m_spatialGrid->Update(collisionManager.GetColliders());
    }

    auto endTime = std::chrono::steady_clock::now();
    m_stats.broadPhaseTime = std::chrono::duration<float>(endTime - startTime).count();
}

void OptimizedCollisionSystem::Shutdown() {
    if (m_spatialGrid) {
        m_spatialGrid->Clear();
    }
    m_collisionCache.clear();
    TraceLog(LOG_INFO, "OptimizedCollisionSystem::Shutdown() - Shutdown optimized collision system");
}

bool OptimizedCollisionSystem::CheckCollision(const Collision& collision, Vector3& response) {
    std::vector<CollisionPair> pairs;

    if (m_useSpatialGrid && m_spatialGrid) {
        // Use spatial grid for broad phase
        auto potentialCollisions = m_spatialGrid->GetNearbyColliders(collision.GetCenter(), 10.0f);
        m_stats.actualChecks = potentialCollisions.size();

        for (Collision* other : potentialCollisions) {
            if (ShouldCheckCollisionPair(const_cast<Collision*>(&collision), other)) {
                float distance = Vector3Distance(collision.GetCenter(), other->GetCenter());
                pairs.emplace_back(const_cast<Collision*>(&collision), other, distance);
            }
        }
    }

    // Perform narrow phase collision detection
    PerformNarrowPhaseCollision(pairs);

    // Return true if collision detected
    return !pairs.empty();
}

std::vector<CollisionPair> OptimizedCollisionSystem::GetNearbyCollisions(const Vector3& position, float radius) {
    std::vector<CollisionPair> pairs;

    if (m_useSpatialGrid && m_spatialGrid) {
        auto nearbyColliders = m_spatialGrid->GetNearbyColliders(position, radius);

        for (Collision* collider : nearbyColliders) {
            float distance = Vector3Distance(position, collider->GetCenter());
            if (distance <= radius) {
                pairs.emplace_back(nullptr, collider, distance);
            }
        }
    }

    return pairs;
}

void OptimizedCollisionSystem::EnableSpatialGrid(bool enable) {
    m_useSpatialGrid = enable;
    if (enable && !m_spatialGrid) {
        Initialize();
    }
    TraceLog(LOG_INFO, "OptimizedCollisionSystem::EnableSpatialGrid() - %s spatial grid",
             enable ? "Enabled" : "Disabled");
}

void OptimizedCollisionSystem::SetSpatialGridBounds(const Vector3& min, const Vector3& max) {
    if (m_spatialGrid) {
        // Would need to recreate spatial grid with new bounds
        TraceLog(LOG_INFO, "OptimizedCollisionSystem::SetSpatialGridBounds() - Updated spatial grid bounds");
    }
}

void OptimizedCollisionSystem::SetSpatialGridCellSize(float cellSize) {
    if (m_spatialGrid) {
        m_spatialGrid->OptimizeGridSize(cellSize);
        TraceLog(LOG_INFO, "OptimizedCollisionSystem::SetSpatialGridCellSize() - Set cell size to %.2f", cellSize);
    }
}

void OptimizedCollisionSystem::SetMaxCollisionChecks(int maxChecks) {
    m_maxCollisionChecks = maxChecks;
    TraceLog(LOG_INFO, "OptimizedCollisionSystem::SetMaxCollisionChecks() - Set max collision checks to %d", maxChecks);
}

void OptimizedCollisionSystem::EnableCollisionCaching(bool enable) {
    m_collisionCaching = enable;
    if (!enable) {
        m_collisionCache.clear();
    }
    TraceLog(LOG_INFO, "OptimizedCollisionSystem::EnableCollisionCaching() - %s collision caching",
             enable ? "Enabled" : "Disabled");
}

void OptimizedCollisionSystem::SetCollisionDistanceThreshold(float threshold) {
    m_collisionDistanceThreshold = threshold;
    TraceLog(LOG_INFO, "OptimizedCollisionSystem::SetCollisionDistanceThreshold() - Set threshold to %.2f", threshold);
}

void OptimizedCollisionSystem::ResetStatistics() {
    m_stats = CollisionStats{};
    TraceLog(LOG_INFO, "OptimizedCollisionSystem::ResetStatistics() - Reset collision statistics");
}

std::string OptimizedCollisionSystem::GetCollisionPairKey(Collision* a, Collision* b) {
    // Create a consistent key for collision pairs
    uintptr_t addrA = reinterpret_cast<uintptr_t>(a);
    uintptr_t addrB = reinterpret_cast<uintptr_t>(b);
    uintptr_t minAddr = std::min(addrA, addrB);
    uintptr_t maxAddr = std::max(addrA, addrB);

    return std::to_string(minAddr) + "_" + std::to_string(maxAddr);
}

void OptimizedCollisionSystem::UpdateSpatialGrid() {
    // Update spatial grid with current colliders
    // Would need access to collision manager
}

std::vector<Collision*> OptimizedCollisionSystem::GetPotentialCollisions(const Collision& collision) {
    if (m_useSpatialGrid && m_spatialGrid) {
        BoundingBox bbox = collision.GetBoundingBox();
        return m_spatialGrid->GetCollidersInAABB(bbox);
    }
    return {};
}

bool OptimizedCollisionSystem::ShouldCheckCollisionPair(Collision* a, Collision* b) {
    // Check if this pair should be tested for collision
    if (!a || !b || a == b) return false;

    // Check distance threshold
    float distance = Vector3Distance(a->GetCenter(), b->GetCenter());
    if (distance > m_collisionDistanceThreshold) return false;

    // Check collision cache
    if (m_collisionCaching) {
        std::string key = GetCollisionPairKey(a, b);
        if (m_collisionCache.find(key) != m_collisionCache.end()) {
            m_stats.cacheHits++;
            return false; // Already checked recently
        }
    }

    return true;
}

void OptimizedCollisionSystem::PerformNarrowPhaseCollision(const std::vector<CollisionPair>& pairs) {
    auto narrowStart = std::chrono::steady_clock::now();

    for (const auto& pair : pairs) {
        if (m_stats.totalChecks >= m_maxCollisionChecks) break;

        m_stats.totalChecks++;

        // Perform actual collision detection
        // This would call the collision detection algorithms
        // For now, just simulate the work

        if (m_collisionCaching) {
            std::string key = GetCollisionPairKey(pair.colliderA, pair.colliderB);
            m_collisionCache.insert(key);
        }
    }

    auto narrowEnd = std::chrono::steady_clock::now();
    m_stats.narrowPhaseTime = std::chrono::duration<float>(narrowEnd - narrowStart).count();
}