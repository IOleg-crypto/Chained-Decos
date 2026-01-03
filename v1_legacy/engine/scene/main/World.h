#pragma once

#include <memory>
#include <raylib.h>
#include <raymath.h>
#include <vector>

class WorldManager
{
public:
    static void Init();
    static void Shutdown();
    static void Update(float deltaTime);

    // World state queries
    static bool IsPointInWorld(const Vector3 &point);
    static bool IsPointOnGround(const Vector3 &point);
    static float GetGroundHeight();

    // Debug
    static void DrawDebugWorld();
    static void ToggleDebugDraw();
    static bool IsDebugDrawEnabled();

    // World constants
    static constexpr float WORLD_FLOOR_Y = -10.0f;
    static constexpr Vector2 GROUND_SIZE = {2000.0f, 2000.0f};
    static constexpr Vector3 GROUND_POSITION = {0.0f, 1.0f, 0.0f};

    WorldManager();
    ~WorldManager() = default;

private:
    bool InternalIsPointInWorld(const Vector3 &point) const;
    bool InternalIsPointOnGround(const Vector3 &point) const;
    void InternalUpdate(float deltaTime);
    void InternalDrawDebugWorld() const;

    void InitializeWorld();
    void LoadWorldGeometry();
    void UpdateWorldBounds();
    void DrawDebugBounds() const;

private:
    bool m_drawDebug = false;
    bool m_worldInitialized = false;

    // World boundaries
    BoundingBox m_worldBounds;
    std::vector<BoundingBox> m_groundSegments;
};
