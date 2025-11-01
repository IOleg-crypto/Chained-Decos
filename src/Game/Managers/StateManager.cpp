#include "StateManager.h"
#include "../Player/Player.h"
#include "../Menu/Menu.h"
#include <raylib.h>

StateManager::StateManager(Player* player, Menu* menu)
    : m_player(player), m_menu(menu)
{
    TraceLog(LOG_INFO, "StateManager created");
}

void StateManager::SaveGameState(const std::string& currentMapPath)
{
    TraceLog(LOG_INFO, "StateManager::SaveGameState() - Saving current game state...");

    m_savedMapPath = currentMapPath;
    TraceLog(LOG_INFO, "StateManager::SaveGameState() - Saved map path: %s", m_savedMapPath.c_str());

    m_savedPlayerPosition = m_player->GetPlayerPosition();
    m_savedPlayerVelocity = m_player->GetPhysics().GetVelocity();
    TraceLog(LOG_INFO, "StateManager::SaveGameState() - Saved player position: (%.2f, %.2f, %.2f)",
             m_savedPlayerPosition.x, m_savedPlayerPosition.y, m_savedPlayerPosition.z);

    m_menu->SetResumeButtonOn(true);

    TraceLog(LOG_INFO, "StateManager::SaveGameState() - Game state saved successfully");
}

void StateManager::RestoreGameState()
{
    TraceLog(LOG_INFO, "StateManager::RestoreGameState() - Restoring game state...");

    if (!m_savedMapPath.empty())
    {
        m_player->SetPlayerPosition(m_savedPlayerPosition);
        m_player->GetPhysics().SetVelocity(m_savedPlayerVelocity);
        TraceLog(LOG_INFO, "StateManager::RestoreGameState() - Restored player position: (%.2f, %.2f, %.2f)",
                 m_savedPlayerPosition.x, m_savedPlayerPosition.y, m_savedPlayerPosition.z);
    }

    TraceLog(LOG_INFO, "StateManager::RestoreGameState() - Game state restored successfully");
}

