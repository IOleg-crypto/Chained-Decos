#include "CheckpointSystem.h"
#include <algorithm>
#include <raymath.h>
#include <raylib.h>

// Define static constexpr members
constexpr float CheckpointSystem::AUTO_CHECKPOINT_DISTANCE;

Checkpoint::Checkpoint(const Vector3& pos, const std::string& checkpointName)
    : position(pos), name(checkpointName), activated(false), activationRadius(3.0f) {
    size = {2.0f, 4.0f, 2.0f}; // Tall checkpoint marker
    color = {100, 200, 255, 200}; // Light blue, semi-transparent
}

void Checkpoint::Activate() {
    if (!activated) {
        activated = true;
        activationTime = std::chrono::steady_clock::now();
        color = {100, 255, 100, 255}; // Bright green when activated
        TraceLog(LOG_INFO, "Checkpoint::Activate() - Activated checkpoint: %s", name.c_str());
    }
}

void Checkpoint::Render() const {
    if (!activated) {
        // Draw inactive checkpoint as wireframe
        DrawCube(position, size.x, size.y, size.z, BLANK);
        DrawCubeWires(position, size.x, size.y, size.z, color);

        // Draw activation radius indicator
        DrawCircle3D({position.x, position.y - size.y * 0.5f, position.z},
                     activationRadius, {1, 0, 0}, 90.0f, Fade(color, 0.3f));
    } else {
        // Draw activated checkpoint as solid
        DrawCube(position, size.x, size.y, size.z, color);

        // Draw activation effect
        float timeSinceActivation = std::chrono::duration<float>(
            std::chrono::steady_clock::now() - activationTime).count();

        if (timeSinceActivation < 2.0f) {
            float pulse = sin(timeSinceActivation * 10.0f) * 0.5f + 0.5f;
            Color pulseColor = {100, 255, 100, static_cast<unsigned char>(255 * pulse)};
            DrawCube(position, size.x * (1.0f + pulse * 0.2f),
                    size.y * (1.0f + pulse * 0.2f),
                    size.z * (1.0f + pulse * 0.2f), pulseColor);
        }

        // Draw checkpoint flag
        Vector3 flagPos = {position.x, position.y + size.y * 0.6f, position.z};
        DrawSphere(flagPos, 0.2f, GREEN);
    }

    // Draw checkpoint name
    Vector3 textPos = {position.x, position.y + size.y * 0.8f, position.z};
    // Note: In a real implementation, you'd want to use proper 3D text rendering
    // For now, we'll skip the text rendering to avoid complexity
}

BoundingBox Checkpoint::GetBoundingBox() const {
    return {
        {position.x - size.x * 0.5f, position.y - size.y * 0.5f, position.z - size.z * 0.5f},
        {position.x + size.x * 0.5f, position.y + size.y * 0.5f, position.z + size.z * 0.5f}
    };
}

bool Checkpoint::IsInRange(const Vector3& playerPos) const {
    float distance = Vector3Distance(position, playerPos);
    return distance <= activationRadius;
}

CheckpointSystem::CheckpointSystem()
    : m_currentCheckpointIndex(0), m_hasRespawnPoint(false), m_autoCheckpoints(false),
      m_lastCheckpointDistance(0.0f) {
    m_respawnPosition = {0.0f, 2.0f, 0.0f}; // Default respawn position
}

void CheckpointSystem::AddCheckpoint(const Vector3& position, const std::string& name) {
    Checkpoint checkpoint(position, name);
    m_checkpoints.push_back(checkpoint);

    TraceLog(LOG_INFO, "CheckpointSystem::AddCheckpoint() - Added checkpoint '%s' at (%.2f, %.2f, %.2f)",
             name.c_str(), position.x, position.y, position.z);
}

void CheckpointSystem::RemoveCheckpoint(size_t index) {
    if (index < m_checkpoints.size()) {
        const std::string& name = m_checkpoints[index].name;
        m_checkpoints.erase(m_checkpoints.begin() + index);

        if (m_currentCheckpointIndex >= m_checkpoints.size() && m_checkpoints.size() > 0) {
            m_currentCheckpointIndex = m_checkpoints.size() - 1;
        }

        TraceLog(LOG_INFO, "CheckpointSystem::RemoveCheckpoint() - Removed checkpoint: %s", name.c_str());
    }
}

void CheckpointSystem::ClearAllCheckpoints() {
    m_checkpoints.clear();
    m_currentCheckpointIndex = 0;
    m_hasRespawnPoint = false;
    TraceLog(LOG_INFO, "CheckpointSystem::ClearAllCheckpoints() - Cleared all checkpoints");
}

void CheckpointSystem::Update(const Vector3& playerPos) {
    UpdateCheckpointVisuals();

    if (m_autoCheckpoints) {
        UpdateAutoCheckpoint(playerPos, AUTO_CHECKPOINT_DISTANCE);
    }
}

void CheckpointSystem::Render() const {
    for (const auto& checkpoint : m_checkpoints) {
        checkpoint.Render();
    }
}

bool CheckpointSystem::CheckPlayerCollision(const Vector3& playerPos) {
    for (size_t i = 0; i < m_checkpoints.size(); ++i) {
        if (m_checkpoints[i].IsInRange(playerPos)) {
            ActivateCheckpoint(i);
            return true;
        }
    }
    return false;
}

void CheckpointSystem::ActivateNearestCheckpoint(const Vector3& playerPos) {
    size_t nearestIndex = FindNearestCheckpointIndex(playerPos);
    if (nearestIndex < m_checkpoints.size()) {
        ActivateCheckpoint(nearestIndex);
    }
}

void CheckpointSystem::ActivateCheckpoint(size_t index) {
    if (index < m_checkpoints.size()) {
        m_checkpoints[index].Activate();
        m_respawnPosition = m_checkpoints[index].position;
        m_hasRespawnPoint = true;
        m_currentCheckpointIndex = index;

        TraceLog(LOG_INFO, "CheckpointSystem::ActivateCheckpoint() - Activated checkpoint %zu: %s",
                 index, m_checkpoints[index].name.c_str());
    }
}

Vector3 CheckpointSystem::GetRespawnPosition() const {
    return m_hasRespawnPoint ? m_respawnPosition : Vector3{0.0f, 2.0f, 0.0f};
}

void CheckpointSystem::SetRespawnPosition(const Vector3& position) {
    m_respawnPosition = position;
    m_hasRespawnPoint = true;
    TraceLog(LOG_INFO, "CheckpointSystem::SetRespawnPosition() - Set respawn position to (%.2f, %.2f, %.2f)",
             position.x, position.y, position.z);
}

bool CheckpointSystem::HasValidRespawnPoint() const {
    return m_hasRespawnPoint;
}

size_t CheckpointSystem::GetActivatedCheckpointCount() const {
    size_t count = 0;
    for (const auto& checkpoint : m_checkpoints) {
        if (checkpoint.activated) count++;
    }
    return count;
}

const Checkpoint* CheckpointSystem::GetLastActivatedCheckpoint() const {
    for (auto it = m_checkpoints.rbegin(); it != m_checkpoints.rend(); ++it) {
        if (it->activated) {
            return &(*it);
        }
    }
    return nullptr;
}

const Checkpoint* CheckpointSystem::GetNearestCheckpoint(const Vector3& position) const {
    size_t nearestIndex = FindNearestCheckpointIndex(position);
    return (nearestIndex < m_checkpoints.size()) ? &m_checkpoints[nearestIndex] : nullptr;
}

void CheckpointSystem::ResetAllCheckpoints() {
    for (auto& checkpoint : m_checkpoints) {
        checkpoint.activated = false;
        checkpoint.color = {100, 200, 255, 200}; // Reset to inactive color
    }
    m_currentCheckpointIndex = 0;
    m_hasRespawnPoint = false;
    TraceLog(LOG_INFO, "CheckpointSystem::ResetAllCheckpoints() - Reset all checkpoints");
}

void CheckpointSystem::SetNextCheckpoint(size_t index) {
    if (index < m_checkpoints.size()) {
        m_currentCheckpointIndex = index;
        TraceLog(LOG_INFO, "CheckpointSystem::SetNextCheckpoint() - Set next checkpoint to %zu", index);
    }
}


void CheckpointSystem::UpdateAutoCheckpoint(const Vector3& playerPos, float distance) {
    if (m_checkpoints.empty()) {
        // Create initial checkpoint at player start position
        AddCheckpoint(playerPos, "Start");
        ActivateCheckpoint(0);
        m_lastCheckpointDistance = 0.0f;
        return;
    }

    m_lastCheckpointDistance += distance;

    if (m_lastCheckpointDistance >= AUTO_CHECKPOINT_DISTANCE) {
        // Create new auto-checkpoint
        std::string name = "Auto-" + std::to_string(m_checkpoints.size() + 1);
        AddCheckpoint(playerPos, name);
        ActivateCheckpoint(m_checkpoints.size() - 1);
        m_lastCheckpointDistance = 0.0f;

        TraceLog(LOG_INFO, "CheckpointSystem::UpdateAutoCheckpoint() - Created auto-checkpoint: %s", name.c_str());
    }
}

void CheckpointSystem::UpdateCheckpointVisuals() {
    // Update visual effects for checkpoints (animations, particles, etc.)
    // This could be expanded with particle effects or other visual enhancements
}

size_t CheckpointSystem::FindNearestCheckpointIndex(const Vector3& position) const {
    if (m_checkpoints.empty()) {
        return static_cast<size_t>(-1);
    }

    size_t nearestIndex = 0;
    float nearestDistance = Vector3Distance(position, m_checkpoints[0].position);

    for (size_t i = 1; i < m_checkpoints.size(); ++i) {
        float distance = Vector3Distance(position, m_checkpoints[i].position);
        if (distance < nearestDistance) {
            nearestDistance = distance;
            nearestIndex = i;
        }
    }

    return nearestIndex;
}