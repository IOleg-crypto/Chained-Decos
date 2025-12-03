import re

def update_iplayer_mediator_h():
    file_path = r'project\chaineddecos\Player\Interfaces\IPlayerMediator.h'
    with open(file_path, 'r', encoding='utf-8') as f:
        content = f.read()
        
    if 'InitializeCollision' not in content:
        content = content.replace('virtual void SyncCollision() const = 0;', 'virtual void SyncCollision() const = 0;\n    virtual void InitializeCollision() = 0;')
        
    with open(file_path, 'w', encoding='utf-8', newline='\r\n') as f:
        f.write(content)
    print("Updated IPlayerMediator.h")

def update_player_h():
    file_path = r'project\chaineddecos\Player\Core\Player.h'
    with open(file_path, 'r', encoding='utf-8') as f:
        content = f.read()
        
    if 'void InitializeCollision() override' not in content:
        # Add override
        # Find a good place. Maybe near SyncCollision.
        content = content.replace('void SyncCollision() const;', 'void SyncCollision() const;\n    void InitializeCollision() override;')
        
    with open(file_path, 'w', encoding='utf-8', newline='\r\n') as f:
        f.write(content)
    print("Updated Player.h")

def update_player_cpp():
    file_path = r'project\chaineddecos\Player\Core\Player.cpp'
    with open(file_path, 'r', encoding='utf-8') as f:
        content = f.read()
        
    if 'void Player::InitializeCollision()' not in content:
        methods = """
void Player::InitializeCollision()
{
    m_collision->InitializeCollision();
}
"""
        content += methods
        
    with open(file_path, 'w', encoding='utf-8', newline='\r\n') as f:
        f.write(content)
    print("Updated Player.cpp")

def update_map_collision_initializer_h():
    file_path = r'project\chaineddecos\Managers\MapCollisionInitializer.h'
    with open(file_path, 'r', encoding='utf-8') as f:
        content = f.read()
        
    # Replace Player* with IPlayerMediator*
    content = content.replace('class Player;', '#include "../Player/Interfaces/IPlayerMediator.h"')
    content = content.replace('Player* player', 'IPlayerMediator* player')
    content = content.replace('Player* m_player', 'IPlayerMediator* m_player')
    
    with open(file_path, 'w', encoding='utf-8', newline='\r\n') as f:
        f.write(content)
    print("Updated MapCollisionInitializer.h")

def update_map_collision_initializer_cpp():
    file_path = r'project\chaineddecos\Managers\MapCollisionInitializer.cpp'
    with open(file_path, 'r', encoding='utf-8') as f:
        content = f.read()
        
    # Replace Player* with IPlayerMediator* in constructor
    content = content.replace('Player *player', 'IPlayerMediator *player')
    
    # Replace SetPlayer signature
    content = content.replace('void MapCollisionInitializer::SetPlayer(Player *player)', 'void MapCollisionInitializer::SetPlayer(IPlayerMediator *player)')
    
    # Replace usage
    # auto &playerCollision = m_player->GetCollisionMutable();
    # playerCollision.InitializeCollision();
    # -> m_player->InitializeCollision();
    
    pattern = r'auto &playerCollision = m_player->GetCollisionMutable\(\);\s*playerCollision\.InitializeCollision\(\);'
    replacement = 'm_player->InitializeCollision();'
    content = re.sub(pattern, replacement, content)
    
    # Remove include Player.h if possible, but might need it?
    # Actually we included IPlayerMediator.h in header.
    # Player.h might be needed if we cast? No casting here.
    # So we can remove #include "project/chaineddecos/Player/Core/Player.h"
    content = content.replace('#include "project/chaineddecos/Player/Core/Player.h"', '')
    
    with open(file_path, 'w', encoding='utf-8', newline='\r\n') as f:
        f.write(content)
    print("Updated MapCollisionInitializer.cpp")

def update_levelmanager_cpp():
    file_path = r'project\chaineddecos\Systems\MapSystem\LevelManager.cpp'
    with open(file_path, 'r', encoding='utf-8') as f:
        content = f.read()
        
    # Fix cast in Update
    # m_player = static_cast<Player*>(player);
    # m_collisionInitializer->SetPlayer(m_player);
    
    # m_player is Player* (member of LevelManager).
    # MapCollisionInitializer::SetPlayer now takes IPlayerMediator*.
    # Player inherits IPlayerMediator.
    # So passing m_player (Player*) to SetPlayer(IPlayerMediator*) is implicit upcast. VALID.
    
    # BUT LevelManager.h defines m_player as Player*.
    # I should probably change LevelManager to hold IPlayer* or IPlayerMediator*?
    # LevelManager uses m_player in SetPlayer(Player* player) too.
    # LevelManager::SetPlayer(Player* player) is public.
    # If I change LevelManager internals, I might break other things?
    # But LevelManager::SetPlayer is likely called by GameApplication?
    # No, GameApplication doesn't call it.
    # LevelManager::Update calls m_engine->GetPlayer() which returns IPlayer*.
    # So LevelManager::Update casts IPlayer* to Player*.
    
    # If I change MapCollisionInitializer to take IPlayerMediator*,
    # and LevelManager passes Player*, it works.
    
    # So LevelManager.cpp doesn't strictly NEED changes, 
    # UNLESS I want to avoid casting to Player* and instead cast to IPlayerMediator*.
    # But LevelManager stores m_player as Player*.
    # I should probably leave LevelManager as is for now (storing Player*), 
    # because changing m_player type might cascade.
    # I'll just let the implicit cast happen when calling SetPlayer.
    
    pass

update_iplayer_mediator_h()
update_player_h()
update_player_cpp()
update_map_collision_initializer_h()
update_map_collision_initializer_cpp()
