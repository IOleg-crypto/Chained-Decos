#ifndef STATE_MANAGER_H
#define STATE_MANAGER_H

#include <string>
#include <raylib.h>
#include "Player/Player.h"
#include "Menu/Menu.h"

class StateManager
{
private:
    std::string m_savedMapPath;
    Vector3 m_savedPlayerPosition;
    Vector3 m_savedPlayerVelocity;
    
    Player* m_player;
    Menu* m_menu;

public:
    StateManager(Player* player, Menu* menu);
    ~StateManager() = default;

    void SaveGameState(const std::string& currentMapPath);
    void RestoreGameState();
    
    const std::string& GetSavedMapPath() const { return m_savedMapPath; }
};

#endif // STATE_MANAGER_H

