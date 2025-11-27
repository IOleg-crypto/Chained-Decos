#ifndef GAME_EVENTS_H
#define GAME_EVENTS_H

#include <raylib.h>
#include <string>

// ============================================================================
// PLAYER EVENTS
// ============================================================================

// Player movement event
struct PlayerMovedEvent
{
    Vector3 oldPosition;
    Vector3 newPosition;
    Vector3 velocity;
};

// Player health changed event
struct PlayerHealthChangedEvent
{
    float oldHealth;
    float newHealth;
    float maxHealth;
    bool isDead;
};

// Player jumped event
struct PlayerJumpedEvent
{
    Vector3 position;
    float jumpForce;
};

// Player attacked event
struct PlayerAttackedEvent
{
    Vector3 position;
    Vector3 direction;
    float damage;
};

// ============================================================================
// MAP EVENTS
// ============================================================================

// Map loaded event
struct MapLoadedEvent
{
    std::string mapPath;
    Vector3 spawnPosition;
};

// Map unloaded event
struct MapUnloadedEvent
{
    std::string mapPath;
};

// ============================================================================
// COLLISION EVENTS
// ============================================================================

// Collision event
struct CollisionEvent
{
    void *objectA;
    void *objectB;
    Vector3 contactPoint;
    Vector3 normal;
    float penetration;
};

// ============================================================================
// INPUT EVENTS
// ============================================================================

// Key pressed event
struct KeyPressedEvent
{
    int key;
};

// Key released event
struct KeyReleasedEvent
{
    int key;
};

// Mouse moved event
struct MouseMovedEvent
{
    Vector2 position;
    Vector2 delta;
};

// ============================================================================
// UI EVENTS
// ============================================================================

// Button clicked event
struct ButtonClickedEvent
{
    std::string buttonId;
};

// Menu opened event
struct MenuOpenedEvent
{
    std::string menuName;
};

// Menu closed event
struct MenuClosedEvent
{
    std::string menuName;
};

// ============================================================================
// RENDERING EVENTS
// ============================================================================

// Render frame event
struct RenderFrameEvent
{
    float deltaTime;
    int frameCount;
};

// ============================================================================
// AUDIO EVENTS
// ============================================================================

// Audio event
struct AudioEvent
{
    std::string soundName;
    Vector3 position;
    float volume;
};

#endif // GAME_EVENTS_H
