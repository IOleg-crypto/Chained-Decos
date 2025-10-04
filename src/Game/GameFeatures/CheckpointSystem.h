#ifndef CHECKPOINT_SYSTEM_H
#define CHECKPOINT_SYSTEM_H

#include <vector>
#include <string>
#include <raylib.h>
#include <chrono>

struct Checkpoint {
    Vector3 position;
    Vector3 size;
    Color color;
    std::string name;
    bool activated;
    float activationRadius;
    std::chrono::steady_clock::time_point activationTime;

    Checkpoint(const Vector3& pos, const std::string& checkpointName = "Checkpoint");
    void Activate();
    void Render() const;
    BoundingBox GetBoundingBox() const;
    bool IsInRange(const Vector3& playerPos) const;
};

class CheckpointSystem {
public:
    CheckpointSystem();
    ~CheckpointSystem() = default;

    // Checkpoint management
    void AddCheckpoint(const Vector3& position, const std::string& name = "Checkpoint");
    void RemoveCheckpoint(size_t index);
    void ClearAllCheckpoints();

    // Update and render
    void Update(const Vector3& playerPos);
    void Render() const;

    // Checkpoint activation
    bool CheckPlayerCollision(const Vector3& playerPos);
    void ActivateNearestCheckpoint(const Vector3& playerPos);
    void ActivateCheckpoint(size_t index);

    // Respawn system
    Vector3 GetRespawnPosition() const;
    void SetRespawnPosition(const Vector3& position);
    bool HasValidRespawnPoint() const;

    // Query methods
    size_t GetCheckpointCount() const { return m_checkpoints.size(); }
    size_t GetActivatedCheckpointCount() const;
    const Checkpoint* GetLastActivatedCheckpoint() const;
    const Checkpoint* GetNearestCheckpoint(const Vector3& position) const;

    // Checkpoint progression
    void ResetAllCheckpoints();
    void SetNextCheckpoint(size_t index);
    size_t GetCurrentCheckpointIndex() const { return m_currentCheckpointIndex; }

    // Auto-checkpoint system
    void EnableAutoCheckpoints(bool enable) { m_autoCheckpoints = enable; }
    bool AreAutoCheckpointsEnabled() const { return m_autoCheckpoints; }
    void UpdateAutoCheckpoint(const Vector3& playerPos, float distance);

private:
    std::vector<Checkpoint> m_checkpoints;
    size_t m_currentCheckpointIndex;
    Vector3 m_respawnPosition;
    bool m_hasRespawnPoint;
    bool m_autoCheckpoints;

    // Auto-checkpoint settings
    float m_lastCheckpointDistance;
    static constexpr float AUTO_CHECKPOINT_DISTANCE = 50.0f; // Distance between auto-checkpoints

    // Helper methods
    void UpdateCheckpointVisuals();
    size_t FindNearestCheckpointIndex(const Vector3& position) const;
};

#endif // CHECKPOINT_SYSTEM_H