#pragma once

#include "../Physics/Components/PhysicsComponent.h"
#include <memory>
#include <raylib.h>
#include <raymath.h>
#include <vector>

#include "../../Kernel/Interfaces/IKernelService.h"

class WorldManager : public IKernelService
{
public:
    // World constants
    static constexpr float WORLD_FLOOR_Y = -10.0f;
    static constexpr Vector2 GROUND_SIZE = {2000.0f, 2000.0f};
    static constexpr Vector3 GROUND_POSITION = {0.0f, 1.0f, 0.0f};

    WorldManager();
    ~WorldManager() = default;

    // World update
    void Update(float deltaTime) override;

    // IKernelService implementation
    bool Initialize() override
    {
        InitializeWorld();
        return true;
    }
    void Shutdown() override
    {
    }
    void Render() override
    {
    }
    const char *GetName() const override
    {
        return "WorldManager";
    }

    // World setup
    void InitializeWorld();
    void LoadWorldGeometry();

    // World state
    bool IsPointInWorld(const Vector3 &point) const;
    bool IsPointOnGround(const Vector3 &point) const;
    float GetGroundHeight() const
    {
        return WORLD_FLOOR_Y;
    }

    // Debug
    void DrawDebugWorld() const;
    void ToggleDebugDraw()
    {
        m_drawDebug = !m_drawDebug;
    }
    bool IsDebugDrawEnabled() const
    {
        return m_drawDebug;
    }

private:
    bool m_drawDebug = false;
    bool m_worldInitialized = false;

    // World boundaries
    BoundingBox m_worldBounds;
    std::vector<BoundingBox> m_groundSegments;

    void UpdateWorldBounds();
    void DrawDebugBounds() const;
};
