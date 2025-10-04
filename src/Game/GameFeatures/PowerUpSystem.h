#ifndef POWERUP_SYSTEM_H
#define POWERUP_SYSTEM_H

#include <vector>
#include <string>
#include <memory>
#include <raylib.h>
#include <chrono>
#include <unordered_map>

enum class PowerUpType {
    SPEED_BOOST,
    JUMP_BOOST,
    GRAVITY_REVERSE,
    SLOW_MOTION,
    INVINCIBILITY,
    DOUBLE_POINTS,
    CHECKPOINT,
    TELEPORT,
    SIZE_CHANGE,
    MAGNETISM
};

struct PowerUpEffect {
    PowerUpType type;
    float duration; // in seconds, 0 = permanent until manually removed
    float intensity; // 0.0 to 1.0
    std::chrono::steady_clock::time_point startTime;
    bool isActive;

    PowerUpEffect(PowerUpType t, float dur, float intens = 1.0f);
    bool IsExpired() const;
    float GetRemainingTime() const;
    float GetElapsedTime() const;
};

struct PowerUp {
    PowerUpType type;
    Vector3 position;
    Vector3 size;
    Color color;
    std::string name;
    std::string description;
    bool collected;
    float rotation;
    float bobOffset;

    PowerUp(PowerUpType t, const Vector3& pos);
    void Update(float deltaTime);
    void Render() const;
    BoundingBox GetBoundingBox() const;
};

class PowerUpSystem {
public:
    PowerUpSystem();
    ~PowerUpSystem() = default;

    // Power-up management
    void AddPowerUp(PowerUpType type, const Vector3& position);
    void RemovePowerUp(size_t index);
    void ClearAllPowerUps();

    // Update and render
    void Update(float deltaTime);
    void Render() const;

    // Effect management
    void ApplyPowerUp(PowerUpType type, float duration = 10.0f, float intensity = 1.0f);
    void RemovePowerUpEffect(PowerUpType type);
    void UpdateActiveEffects();

    // Query methods
    bool HasActiveEffect(PowerUpType type) const;
    const PowerUpEffect* GetActiveEffect(PowerUpType type) const;
    float GetEffectIntensity(PowerUpType type) const;
    std::vector<PowerUpEffect> GetActiveEffects() const;

    // Collision detection
    bool CheckPlayerCollision(const Vector3& playerPos, const Vector3& playerSize);
    size_t GetCollectedPowerUpCount() const;

    // Power-up spawning
    void SpawnRandomPowerUps(int count, const Vector3& areaCenter, float areaRadius);
    void SpawnPowerUpRing(int count, const Vector3& center, float radius, float height);

private:
    std::vector<PowerUp> m_powerUps;
    std::vector<PowerUpEffect> m_activeEffects;

    // Power-up properties
    static const std::unordered_map<PowerUpType, Color> POWERUP_COLORS;
    static const std::unordered_map<PowerUpType, std::string> POWERUP_NAMES;
    static const std::unordered_map<PowerUpType, std::string> POWERUP_DESCRIPTIONS;

    // Helper methods
    Color GetPowerUpColor(PowerUpType type) const;
    std::string GetPowerUpName(PowerUpType type) const;
    std::string GetPowerUpDescription(PowerUpType type) const;
    Vector3 GetPowerUpSize(PowerUpType type) const;
};

#endif // POWERUP_SYSTEM_H