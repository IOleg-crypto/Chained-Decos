#ifndef GAME_EVENTS_H
#define GAME_EVENTS_H

#include "core/object/event/Core/Event.h"
#include <raylib.h>
#include <string>

// ============================================================================
// Map-related Events
// ============================================================================

// Event fired when a map is successfully loaded
class MapLoadedEvent : public Event
{
public:
    explicit MapLoadedEvent(const std::string &mapPath, Vector3 spawnPosition)
        : m_mapPath(mapPath), m_spawnPosition(spawnPosition)
    {
    }

    DEFINE_EVENT_TYPE(MapLoadedEvent)

    const std::string &GetMapPath() const
    {
        return m_mapPath;
    }
    Vector3 GetSpawnPosition() const
    {
        return m_spawnPosition;
    }

private:
    std::string m_mapPath;
    Vector3 m_spawnPosition;
};

// Event fired when map loading fails
class MapLoadFailedEvent : public Event
{
public:
    explicit MapLoadFailedEvent(const std::string &mapPath, const std::string &errorMessage)
        : m_mapPath(mapPath), m_errorMessage(errorMessage)
    {
    }

    DEFINE_EVENT_TYPE(MapLoadFailedEvent)

    const std::string &GetMapPath() const
    {
        return m_mapPath;
    }
    const std::string &GetErrorMessage() const
    {
        return m_errorMessage;
    }

private:
    std::string m_mapPath;
    std::string m_errorMessage;
};

// Event fired when map is unloaded
class MapUnloadedEvent : public Event
{
public:
    MapUnloadedEvent() = default;

    DEFINE_EVENT_TYPE(MapUnloadedEvent)
};

// ============================================================================
// Player-related Events
// ============================================================================

// Event fired when player spawns
class PlayerSpawnedEvent : public Event
{
public:
    explicit PlayerSpawnedEvent(Vector3 position) : m_position(position)
    {
    }

    DEFINE_EVENT_TYPE(PlayerSpawnedEvent)

    Vector3 GetPosition() const
    {
        return m_position;
    }

private:
    Vector3 m_position;
};

// Event fired when player dies/respawns
class PlayerRespawnedEvent : public Event
{
public:
    explicit PlayerRespawnedEvent(Vector3 position) : m_position(position)
    {
    }

    DEFINE_EVENT_TYPE(PlayerRespawnedEvent)

    Vector3 GetPosition() const
    {
        return m_position;
    }

private:
    Vector3 m_position;
};

// Event fired when player state is saved
class PlayerStateSavedEvent : public Event
{
public:
    explicit PlayerStateSavedEvent(const std::string &mapPath, Vector3 position)
        : m_mapPath(mapPath), m_position(position)
    {
    }

    DEFINE_EVENT_TYPE(PlayerStateSavedEvent)

    const std::string &GetMapPath() const
    {
        return m_mapPath;
    }
    Vector3 GetPosition() const
    {
        return m_position;
    }

private:
    std::string m_mapPath;
    Vector3 m_position;
};

// ============================================================================
// UI-related Events
// ============================================================================

// Event fired when menu is opened
class MenuOpenedEvent : public Event
{
public:
    MenuOpenedEvent() = default;

    DEFINE_EVENT_TYPE(MenuOpenedEvent)
};

// Event fired when menu is closed
class MenuClosedEvent : public Event
{
public:
    MenuClosedEvent() = default;

    DEFINE_EVENT_TYPE(MenuClosedEvent)
};

// Event fired when game is paused
class GamePausedEvent : public Event
{
public:
    GamePausedEvent() = default;

    DEFINE_EVENT_TYPE(GamePausedEvent)
};

// Event fired when game is resumed
class GameResumedEvent : public Event
{
public:
    GameResumedEvent() = default;

    DEFINE_EVENT_TYPE(GameResumedEvent)
};

#endif // GAME_EVENTS_H
