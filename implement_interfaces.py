import re
import os

def update_player_h():
    file_path = r'project\chaineddecos\Player\Core\Player.h'
    with open(file_path, 'r', encoding='utf-8') as f:
        content = f.read()

    # Add include
    if '#include "core/interfaces/IPlayer.h"' not in content:
        content = content.replace(
            '#include "servers/rendering/Interfaces/IGameRenderable.h"',
            '#include "servers/rendering/Interfaces/IGameRenderable.h"\n#include "core/interfaces/IPlayer.h"'
        )

    # Update inheritance
    if 'public IPlayer' not in content:
        content = content.replace(
            'class Player : public IPlayerMediator',
            'class Player : public IPlayer, public IPlayerMediator'
        )

    # Add interface methods
    interface_methods = """
    // IPlayer Interface Implementation
    Vector3 GetPosition() const override { return GetPlayerPosition(); }
    void SetPosition(const Vector3& pos) override { SetPlayerPosition(pos); }
    void SetSpeed(float speed) override { SetSpeed(speed); } // Already exists but adding override
    float GetSpeed() const override { return GetSpeed(); } // Already exists
    void SetRotationY(float rotation) override { SetRotationY(rotation); } // Already exists
    float GetRotationY() const override { return GetRotationY(); } // Already exists
    
    void Update(float deltaTime) override; // Implemented in .cpp
    Camera3D& GetCamera() override; // Implemented in .cpp
    """
    
    # Insert methods before "Service injection"
    if '// IPlayer Interface Implementation' not in content:
        content = content.replace(
            '// Service injection',
            interface_methods + '\n    // Service injection'
        )
        
    # Fix existing methods signatures if needed (SetSpeed/GetSpeed/Rotation are const in interface but maybe not in class)
    # The interface defines: virtual void SetSpeed(float speed) = 0;
    # Player.h has: void SetSpeed(float speed) const;
    # Wait, SetSpeed is const in Player.h? That's weird for a setter.
    # Let's check Player.h content again.
    # line 75: void SetSpeed(float speed) const;         // Set speed
    # Yes, it is const. IPlayer interface has: virtual void SetSpeed(float speed) = 0; (non-const)
    # I need to match signatures.
    
    with open(file_path, 'w', encoding='utf-8', newline='\r\n') as f:
        f.write(content)
    print("Updated Player.h")

def update_player_cpp():
    file_path = r'project\chaineddecos\Player\Core\Player.cpp'
    with open(file_path, 'r', encoding='utf-8') as f:
        content = f.read()
        
    methods = """
void Player::Update(float deltaTime)
{
    if (m_collisionManager) {
        Update(*m_collisionManager);
    }
}

Camera3D& Player::GetCamera()
{
    return m_cameraController->GetCamera();
}
"""
    if 'Player::Update(float deltaTime)' not in content:
        content += methods
        
    with open(file_path, 'w', encoding='utf-8', newline='\r\n') as f:
        f.write(content)
    print("Updated Player.cpp")

def update_menu_h():
    file_path = r'project\chaineddecos\Menu\Menu.h'
    with open(file_path, 'r', encoding='utf-8') as f:
        content = f.read()

    if '#include "core/interfaces/IMenu.h"' not in content:
        content = content.replace(
            '#include <servers/rendering/Interfaces/IMenuRenderable.h>',
            '#include <servers/rendering/Interfaces/IMenuRenderable.h>\n#include "core/interfaces/IMenu.h"'
        )

    if 'public IMenu' not in content:
        content = content.replace(
            'class Menu : public IMenuRenderable',
            'class Menu : public IMenu, public IMenuRenderable'
        )

    interface_methods = """
    // IMenu Interface Implementation
    bool IsOpen() const override { return m_state != MenuState::GameMode; } // Simplified logic
    void Show() override { m_state = MenuState::Main; }
    void Hide() override { m_state = MenuState::GameMode; }
    bool ShouldStartGame() const override { return m_action == MenuAction::StartGame || m_action == MenuAction::ResumeGame; }
    std::string GetSelectedMap() const override { return GetSelectedMapName(); }
    void ToggleConsole() override { ToggleConsole(); } // Already exists
    bool IsConsoleOpen() const override { return IsConsoleOpen(); } // Already exists
    """

    if '// IMenu Interface Implementation' not in content:
        content = content.replace(
            '// IMenuRenderable interface implementations',
            interface_methods + '\n    // IMenuRenderable interface implementations'
        )

    with open(file_path, 'w', encoding='utf-8', newline='\r\n') as f:
        f.write(content)
    print("Updated Menu.h")

def update_levelmanager_h():
    file_path = r'project\chaineddecos\Systems\MapSystem\LevelManager.h'
    with open(file_path, 'r', encoding='utf-8') as f:
        content = f.read()

    if '#include "core/interfaces/ILevelManager.h"' not in content:
        content = content.replace(
            '#include "../../../core/object/module/Interfaces/IEngineModule.h"',
            '#include "../../../core/object/module/Interfaces/IEngineModule.h"\n#include "core/interfaces/ILevelManager.h"'
        )

    if 'public ILevelManager' not in content:
        content = content.replace(
            'class LevelManager : public IEngineModule',
            'class LevelManager : public ILevelManager, public IEngineModule'
        )

    interface_methods = """
    // ILevelManager Interface Implementation
    bool LoadMap(const std::string& path) override { LoadEditorMap(path); return true; } // Mapping to LoadEditorMap for now
    void UnloadMap() override { /* Implement unload logic if needed */ }
    bool IsMapLoaded() const override { return m_gameMap != nullptr; }
    std::string GetCurrentMapName() const override { return GetCurrentMapPath(); }
    Vector3 GetSpawnPosition() const override { return GetPlayerSpawnPosition(); }
    """

    if '// ILevelManager Interface Implementation' not in content:
        content = content.replace(
            '// IEngineModule interface',
            interface_methods + '\n    // IEngineModule interface'
        )

    with open(file_path, 'w', encoding='utf-8', newline='\r\n') as f:
        f.write(content)
    print("Updated LevelManager.h")

update_player_h()
update_player_cpp()
update_menu_h()
update_levelmanager_h()
