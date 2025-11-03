#include "GameRenderHelpers.h"
#include "Engine/Collision/CollisionManager.h"
#include <raylib.h>
#include <algorithm>

GameRenderHelpers::GameRenderHelpers(CollisionManager* collisionManager)
    : m_collisionManager(collisionManager)
{
    TraceLog(LOG_INFO, "GameRenderHelpers created");
}

void GameRenderHelpers::CreatePlatform(const Vector3 &position, const Vector3 &size, Color color, CollisionType collisionType)
{
    DrawCube(position, size.x, size.y, size.z, color);

    Collision collision(position, size);
    collision.SetCollisionType(collisionType);
    m_collisionManager->AddCollider(std::move(collision));
}

float GameRenderHelpers::CalculateDynamicFontSize(float baseSize)
{
    int screenWidth = GetScreenWidth();
    float scaleFactor = static_cast<float>(screenWidth) / 1920.0f;
    float dynamicSize = baseSize * scaleFactor;

    constexpr float minSize = 12.0f;
    constexpr float maxSize = 72.0f;

    return std::clamp(dynamicSize, minSize, maxSize);
}

