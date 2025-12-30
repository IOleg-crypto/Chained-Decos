#include "World.h"



WorldManager::WorldManager() {
    InitializeWorld();
}

void WorldManager::InitializeWorld() {
    if (m_worldInitialized) return;

    LoadWorldGeometry();
    UpdateWorldBounds();
    m_worldInitialized = true;
}

void WorldManager::LoadWorldGeometry() {
    // Create main ground segment
    BoundingBox groundBox = {
        {-GROUND_SIZE.x / 2, WORLD_FLOOR_Y, -GROUND_SIZE.y / 2},  // min
        {GROUND_SIZE.x / 2, WORLD_FLOOR_Y + 2.0f, GROUND_SIZE.y / 2}   // max
    };
    m_groundSegments.push_back(groundBox);
    
    UpdateWorldBounds();
}

void WorldManager::Update(float deltaTime) {
    if (!m_worldInitialized) {
        InitializeWorld();
    }
}

void WorldManager::UpdateWorldBounds() {
    if (m_groundSegments.empty()) {
        m_worldBounds = {{0,0,0}, {0,0,0}};
        return;
    }

    // Start with first segment's bounds
    m_worldBounds = m_groundSegments[0];

    // Expand to include all ground segments
    for (size_t i = 1; i < m_groundSegments.size(); i++) {
        const auto& segment = m_groundSegments[i];
        m_worldBounds.min = Vector3{
            std::min(m_worldBounds.min.x, segment.min.x),
            std::min(m_worldBounds.min.y, segment.min.y),
            std::min(m_worldBounds.min.z, segment.min.z)
        };
        m_worldBounds.max = Vector3{
            std::max(m_worldBounds.max.x, segment.max.x),
            std::max(m_worldBounds.max.y, segment.max.y),
            std::max(m_worldBounds.max.z, segment.max.z)
        };
    }
}

bool WorldManager::IsPointInWorld(const Vector3& point) const {
    return point.x >= m_worldBounds.min.x && point.x <= m_worldBounds.max.x &&
           point.y >= m_worldBounds.min.y && point.y <= m_worldBounds.max.y &&
           point.z >= m_worldBounds.min.z && point.z <= m_worldBounds.max.z;
}

bool WorldManager::IsPointOnGround(const Vector3& point) const {
    for (const auto& groundBox : m_groundSegments) {
        if (point.x >= groundBox.min.x && point.x <= groundBox.max.x &&
            point.z >= groundBox.min.z && point.z <= groundBox.max.z) {
            return std::abs(point.y - groundBox.min.y) < 0.1f;
        }
    }
    return false;
}

void WorldManager::DrawDebugWorld() const {
    if (!m_drawDebug) return;
    DrawDebugBounds();
}

void WorldManager::DrawDebugBounds() const {
    // Draw world bounds
    DrawBoundingBox(m_worldBounds, GREEN);

    // Draw ground segments
    for (const auto& groundBox : m_groundSegments) {
        DrawBoundingBox(groundBox, BLUE);
    }
}





