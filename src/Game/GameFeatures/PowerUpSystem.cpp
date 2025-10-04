#include "PowerUpSystem.h"
#include <algorithm>
#include <random>
#include <raymath.h>
#include <raylib.h>

PowerUpEffect::PowerUpEffect(PowerUpType t, float dur, float intens)
    : type(t), duration(dur), intensity(intens), startTime(std::chrono::steady_clock::now()),
      isActive(true) {}

bool PowerUpEffect::IsExpired() const {
    if (duration <= 0.0f) return false; // Permanent effect
    return GetElapsedTime() >= duration;
}

float PowerUpEffect::GetRemainingTime() const {
    if (duration <= 0.0f) return -1.0f; // Permanent
    float elapsed = GetElapsedTime();
    return (elapsed >= duration) ? 0.0f : (duration - elapsed);
}

float PowerUpEffect::GetElapsedTime() const {
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration<float>(now - startTime).count();
}

PowerUp::PowerUp(PowerUpType t, const Vector3& pos)
    : type(t), position(pos), collected(false), rotation(0.0f), bobOffset(0.0f) {
    size = {1.0f, 1.0f, 1.0f}; // Default size
    color = {255, 255, 255, 255}; // Will be set based on type
    name = "Unknown";
    description = "Unknown power-up";
}

void PowerUp::Update(float deltaTime) {
    rotation += 90.0f * deltaTime; // Rotate 90 degrees per second
    bobOffset += deltaTime * 2.0f; // Bob up and down

    if (rotation > 360.0f) rotation -= 360.0f;
    if (bobOffset > 2.0f * PI) bobOffset -= 2.0f * PI;
}

void PowerUp::Render() const {
    if (collected) return;

    Vector3 renderPos = position;
    renderPos.y += sin(bobOffset) * 0.3f; // Bob up and down

    // Draw main power-up cube
    DrawCube(renderPos, size.x, size.y, size.z, color);

    // Draw wireframe outline
    DrawCubeWires(renderPos, size.x, size.y, size.z, WHITE);

    // Draw glow effect
    Color glowColor = {color.r, color.g, color.b, 128};
    DrawCube(renderPos, size.x * 1.2f, size.y * 1.2f, size.z * 1.2f, glowColor);

    // Draw rotation indicator
    Vector3 indicatorPos = {renderPos.x, renderPos.y + 1.5f, renderPos.z};
    DrawSphere(indicatorPos, 0.1f, YELLOW);
}

BoundingBox PowerUp::GetBoundingBox() const {
    return {
        {position.x - size.x * 0.5f, position.y - size.y * 0.5f, position.z - size.z * 0.5f},
        {position.x + size.x * 0.5f, position.y + size.y * 0.5f, position.z + size.z * 0.5f}
    };
}

PowerUpSystem::PowerUpSystem() {
    // Initialize static data if needed
}

void PowerUpSystem::AddPowerUp(PowerUpType type, const Vector3& position) {
    PowerUp powerUp(type, position);
    powerUp.color = GetPowerUpColor(type);
    powerUp.name = GetPowerUpName(type);
    powerUp.description = GetPowerUpDescription(type);
    powerUp.size = GetPowerUpSize(type);

    m_powerUps.push_back(powerUp);
    TraceLog(LOG_INFO, "PowerUpSystem::AddPowerUp() - Added %s at (%.2f, %.2f, %.2f)",
             powerUp.name.c_str(), position.x, position.y, position.z);
}

void PowerUpSystem::RemovePowerUp(size_t index) {
    if (index < m_powerUps.size()) {
        m_powerUps.erase(m_powerUps.begin() + index);
    }
}

void PowerUpSystem::ClearAllPowerUps() {
    m_powerUps.clear();
    TraceLog(LOG_INFO, "PowerUpSystem::ClearAllPowerUps() - Cleared all power-ups");
}

void PowerUpSystem::Update(float deltaTime) {
    // Update power-ups
    for (auto& powerUp : m_powerUps) {
        powerUp.Update(deltaTime);
    }

    // Update active effects
    UpdateActiveEffects();
}

void PowerUpSystem::Render() const {
    for (const auto& powerUp : m_powerUps) {
        powerUp.Render();
    }
}

void PowerUpSystem::ApplyPowerUp(PowerUpType type, float duration, float intensity) {
    // Remove existing effect of same type
    RemovePowerUpEffect(type);

    PowerUpEffect effect(type, duration, intensity);
    m_activeEffects.push_back(effect);

    TraceLog(LOG_INFO, "PowerUpSystem::ApplyPowerUp() - Applied %s for %.1f seconds",
             GetPowerUpName(type).c_str(), duration);
}

void PowerUpSystem::RemovePowerUpEffect(PowerUpType type) {
    m_activeEffects.erase(
        std::remove_if(m_activeEffects.begin(), m_activeEffects.end(),
                       [type](const PowerUpEffect& effect) { return effect.type == type; }),
        m_activeEffects.end()
    );
}

void PowerUpSystem::UpdateActiveEffects() {
    // Remove expired effects
    m_activeEffects.erase(
        std::remove_if(m_activeEffects.begin(), m_activeEffects.end(),
                       [](const PowerUpEffect& effect) { return effect.IsExpired(); }),
        m_activeEffects.end()
    );

    // Update remaining time for active effects
    for (auto& effect : m_activeEffects) {
        effect.isActive = !effect.IsExpired();
    }
}

bool PowerUpSystem::HasActiveEffect(PowerUpType type) const {
    auto it = std::find_if(m_activeEffects.begin(), m_activeEffects.end(),
                          [type](const PowerUpEffect& effect) {
                              return effect.type == type && effect.isActive;
                          });
    return it != m_activeEffects.end();
}

const PowerUpEffect* PowerUpSystem::GetActiveEffect(PowerUpType type) const {
    auto it = std::find_if(m_activeEffects.begin(), m_activeEffects.end(),
                          [type](const PowerUpEffect& effect) {
                              return effect.type == type && effect.isActive;
                          });
    return (it != m_activeEffects.end()) ? &(*it) : nullptr;
}

float PowerUpSystem::GetEffectIntensity(PowerUpType type) const {
    const PowerUpEffect* effect = GetActiveEffect(type);
    return (effect != nullptr) ? effect->intensity : 0.0f;
}

std::vector<PowerUpEffect> PowerUpSystem::GetActiveEffects() const {
    std::vector<PowerUpEffect> active;
    for (const auto& effect : m_activeEffects) {
        if (effect.isActive) {
            active.push_back(effect);
        }
    }
    return active;
}

bool PowerUpSystem::CheckPlayerCollision(const Vector3& playerPos, const Vector3& playerSize) {
    for (size_t i = 0; i < m_powerUps.size(); ++i) {
        if (m_powerUps[i].collected) continue;

        BoundingBox powerUpBox = m_powerUps[i].GetBoundingBox();
        BoundingBox playerBox = {
            {playerPos.x - playerSize.x * 0.5f, playerPos.y - playerSize.y * 0.5f, playerPos.z - playerSize.z * 0.5f},
            {playerPos.x + playerSize.x * 0.5f, playerPos.y + playerSize.y * 0.5f, playerPos.z + playerSize.z * 0.5f}
        };

        if (CheckCollisionBoxes(powerUpBox, playerBox)) {
            // Collect the power-up
            PowerUpType type = m_powerUps[i].type;
            ApplyPowerUp(type, 15.0f); // Default 15 second duration
            m_powerUps[i].collected = true;

            TraceLog(LOG_INFO, "PowerUpSystem::CheckPlayerCollision() - Collected %s",
                     GetPowerUpName(type).c_str());
            return true;
        }
    }
    return false;
}

size_t PowerUpSystem::GetCollectedPowerUpCount() const {
    size_t count = 0;
    for (const auto& powerUp : m_powerUps) {
        if (powerUp.collected) count++;
    }
    return count;
}

void PowerUpSystem::SpawnRandomPowerUps(int count, const Vector3& areaCenter, float areaRadius) {
    std::vector<PowerUpType> types = {
        PowerUpType::SPEED_BOOST, PowerUpType::JUMP_BOOST, PowerUpType::DOUBLE_POINTS,
        PowerUpType::SLOW_MOTION, PowerUpType::INVINCIBILITY
    };

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist(-areaRadius, areaRadius);

    for (int i = 0; i < count; ++i) {
        Vector3 position = {
            areaCenter.x + dist(gen),
            areaCenter.y + 5.0f, // Spawn above ground
            areaCenter.z + dist(gen)
        };

        PowerUpType randomType = types[gen() % types.size()];
        AddPowerUp(randomType, position);
    }

    TraceLog(LOG_INFO, "PowerUpSystem::SpawnRandomPowerUps() - Spawned %d random power-ups", count);
}

void PowerUpSystem::SpawnPowerUpRing(int count, const Vector3& center, float radius, float height) {
    float angleStep = 360.0f / count;

    for (int i = 0; i < count; ++i) {
        float angle = angleStep * i * DEG2RAD;
        Vector3 position = {
            center.x + cos(angle) * radius,
            center.y + height,
            center.z + sin(angle) * radius
        };

        PowerUpType type = static_cast<PowerUpType>(i % static_cast<int>(PowerUpType::MAGNETISM));
        AddPowerUp(type, position);
    }

    TraceLog(LOG_INFO, "PowerUpSystem::SpawnPowerUpRing() - Spawned %d power-ups in ring formation", count);
}

Color PowerUpSystem::GetPowerUpColor(PowerUpType type) const {
    static const std::unordered_map<PowerUpType, Color> colors = {
        {PowerUpType::SPEED_BOOST, {255, 100, 100, 255}},    // Red
        {PowerUpType::JUMP_BOOST, {100, 255, 100, 255}},     // Green
        {PowerUpType::GRAVITY_REVERSE, {100, 100, 255, 255}}, // Blue
        {PowerUpType::SLOW_MOTION, {255, 255, 100, 255}},    // Yellow
        {PowerUpType::INVINCIBILITY, {255, 100, 255, 255}},  // Magenta
        {PowerUpType::DOUBLE_POINTS, {255, 150, 50, 255}},   // Orange
        {PowerUpType::CHECKPOINT, {50, 255, 150, 255}},      // Cyan
        {PowerUpType::TELEPORT, {150, 50, 255, 255}},       // Purple
        {PowerUpType::SIZE_CHANGE, {255, 50, 150, 255}},     // Pink
        {PowerUpType::MAGNETISM, {50, 150, 255, 255}}        // Light Blue
    };

    auto it = colors.find(type);
    return (it != colors.end()) ? it->second : WHITE;
}

std::string PowerUpSystem::GetPowerUpName(PowerUpType type) const {
    static const std::unordered_map<PowerUpType, std::string> names = {
        {PowerUpType::SPEED_BOOST, "Speed Boost"},
        {PowerUpType::JUMP_BOOST, "Jump Boost"},
        {PowerUpType::GRAVITY_REVERSE, "Gravity Reverse"},
        {PowerUpType::SLOW_MOTION, "Slow Motion"},
        {PowerUpType::INVINCIBILITY, "Invincibility"},
        {PowerUpType::DOUBLE_POINTS, "Double Points"},
        {PowerUpType::CHECKPOINT, "Checkpoint"},
        {PowerUpType::TELEPORT, "Teleport"},
        {PowerUpType::SIZE_CHANGE, "Size Change"},
        {PowerUpType::MAGNETISM, "Magnetism"}
    };

    auto it = names.find(type);
    return (it != names.end()) ? it->second : "Unknown";
}

std::string PowerUpSystem::GetPowerUpDescription(PowerUpType type) const {
    static const std::unordered_map<PowerUpType, std::string> descriptions = {
        {PowerUpType::SPEED_BOOST, "Increases movement speed by 50%"},
        {PowerUpType::JUMP_BOOST, "Increases jump height by 100%"},
        {PowerUpType::GRAVITY_REVERSE, "Reverses gravity for 10 seconds"},
        {PowerUpType::SLOW_MOTION, "Slows down time by 50%"},
        {PowerUpType::INVINCIBILITY, "Prevents damage for 15 seconds"},
        {PowerUpType::DOUBLE_POINTS, "Doubles all points earned"},
        {PowerUpType::CHECKPOINT, "Creates a respawn point"},
        {PowerUpType::TELEPORT, "Teleports to a random location"},
        {PowerUpType::SIZE_CHANGE, "Changes player size"},
        {PowerUpType::MAGNETISM, "Attracts nearby collectibles"}
    };

    auto it = descriptions.find(type);
    return (it != descriptions.end()) ? it->second : "Unknown effect";
}

Vector3 PowerUpSystem::GetPowerUpSize(PowerUpType type) const {
    switch (type) {
        case PowerUpType::CHECKPOINT:
            return {2.0f, 2.0f, 2.0f}; // Larger for checkpoints
        case PowerUpType::TELEPORT:
            return {1.5f, 1.5f, 1.5f}; // Medium for teleports
        default:
            return {1.0f, 1.0f, 1.0f}; // Standard size
    }
}