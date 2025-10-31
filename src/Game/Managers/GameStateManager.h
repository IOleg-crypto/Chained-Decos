#ifndef GAME_STATE_MANAGER_H
#define GAME_STATE_MANAGER_H

#include <string>
#include <raylib.h>

class Player;
class Menu;

class GameStateManager
{
private:
    std::string m_savedMapPath;
    Vector3 m_savedPlayerPosition;
    Vector3 m_savedPlayerVelocity;
    
    Player* m_player;
    Menu* m_menu;

public:
    GameStateManager(Player* player, Menu* menu);
    ~GameStateManager() = default;

    void SaveGameState(const std::string& currentMapPath);
    void RestoreGameState();
    
    const std::string& GetSavedMapPath() const { return m_savedMapPath; }
};

#endif // GAME_STATE_MANAGER_H

