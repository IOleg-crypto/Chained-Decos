import re

def update_player_cpp():
    file_path = r'project\chaineddecos\Player\Core\Player.cpp'
    with open(file_path, 'r', encoding='utf-8') as f:
        content = f.read()
        
    methods = """
void Player::SetNoclip(bool enabled)
{
    GetCollisionMutable().EnableBVHCollision(enabled);
}

bool Player::IsNoclip() const
{
    return GetCollision().IsUsingBVH();
}
"""
    if 'Player::SetNoclip' not in content:
        content += methods
        
    with open(file_path, 'w', encoding='utf-8', newline='\r\n') as f:
        f.write(content)
    print("Updated Player.cpp")

def fix_console_manager():
    file_path = r'project\chaineddecos\Menu\Console\ConsoleManager.cpp'
    with open(file_path, 'r', encoding='utf-8') as f:
        content = f.read()
        
    # Fix noclip command
    # Old:
    # PlayerCollision &collision = player->GetCollisionMutable();
    # bool current = collision.IsUsingBVH();
    # collision.EnableBVHCollision(!current);
    # console->AddOutput("Noclip: " + std::string(!current ? "enabled" : "disabled"));
    
    # New:
    # bool current = player->IsNoclip();
    # player->SetNoclip(!current);
    # console->AddOutput("Noclip: " + std::string(!current ? "enabled" : "disabled"));
    
    # Regex replacement for noclip body
    noclip_pattern = r'PlayerCollision &collision = player->GetCollisionMutable\(\);.*?console->AddOutput\("Noclip: " \+.*?std::string\(!current \? "enabled" : "disabled"\)\);'
    noclip_replacement = """bool current = player->IsNoclip();
                        player->SetNoclip(!current);
                        console->AddOutput("Noclip: " +
                                           std::string(!current ? "enabled" : "disabled"));"""
                                           
    content = re.sub(noclip_pattern, noclip_replacement, content, flags=re.DOTALL)
    
    # Fix cl_speed
    # Old: player->GetMovement()->SetSpeed(speed);
    # New: player->SetSpeed(speed);
    content = content.replace('player->GetMovement()->SetSpeed(speed);', 'player->SetSpeed(speed);')
    
    # Fix cl_setpos
    # Old: player->SetPlayerPosition({x, y, z});
    # New: player->SetPosition({x, y, z});
    content = content.replace('player->SetPlayerPosition({x, y, z});', 'player->SetPosition({x, y, z});')
    
    # Fix cl_getpos
    # Old: Vector3 pos = player->GetPlayerPosition();
    # New: Vector3 pos = player->GetPosition();
    content = content.replace('Vector3 pos = player->GetPlayerPosition();', 'Vector3 pos = player->GetPosition();')
    
    # Remove include for PlayerCollision.h if not needed
    # It was: #include "project/chaineddecos/Player/Collision/PlayerCollision.h"
    # I can remove it.
    content = content.replace('#include "project/chaineddecos/Player/Collision/PlayerCollision.h"', '')
    
    # Change static_cast<Player*> to IPlayer*?
    # No, ConsoleManager still casts to Player* in GetPlayer().
    # But now it only uses IPlayer methods!
    # So I can change GetPlayer() to return IPlayer* and remove the cast!
    
    # Change GetPlayer signature in ConsoleManager.h/cpp
    # But wait, GetPlayer() in ConsoleManager.cpp calls Engine::Instance().GetPlayer() which returns IPlayer*.
    # So I can just return IPlayer*.
    
    with open(file_path, 'w', encoding='utf-8', newline='\r\n') as f:
        f.write(content)
    print("Fixed ConsoleManager.cpp")

def update_console_manager_h():
    file_path = r'project\chaineddecos\Menu\Console\ConsoleManager.h'
    with open(file_path, 'r', encoding='utf-8') as f:
        content = f.read()
        
    # Change return type of GetPlayer
    content = content.replace('class Player;', '#include "core/interfaces/IPlayer.h"') # Replace fwd decl with include? Or just use IPlayer*
    # Actually, IPlayer is already included in Engine.h, but ConsoleManager.h might need it if we return IPlayer*.
    # Better to include IPlayer.h
    
    if '#include "core/interfaces/IPlayer.h"' not in content:
        content = content.replace('class Player;', '#include "core/interfaces/IPlayer.h"')
        
    content = content.replace('Player *GetPlayer() const;', 'IPlayer *GetPlayer() const;')
    
    with open(file_path, 'w', encoding='utf-8', newline='\r\n') as f:
        f.write(content)
    print("Updated ConsoleManager.h")

def update_console_manager_cpp_return_type():
    file_path = r'project\chaineddecos\Menu\Console\ConsoleManager.cpp'
    with open(file_path, 'r', encoding='utf-8') as f:
        content = f.read()
        
    content = content.replace('Player *ConsoleManager::GetPlayer() const', 'IPlayer *ConsoleManager::GetPlayer() const')
    # Remove the static_cast I added previously
    content = content.replace('auto player = static_cast<Player*>(Engine::Instance().GetPlayer());', 'auto player = Engine::Instance().GetPlayer();')
    
    # Also fix usages of Player* in lambdas
    # Player *player = console->GetPlayer(); -> IPlayer *player = console->GetPlayer();
    content = content.replace('Player *player = console->GetPlayer();', 'IPlayer *player = console->GetPlayer();')
    
    with open(file_path, 'w', encoding='utf-8', newline='\r\n') as f:
        f.write(content)
    print("Updated ConsoleManager.cpp return type")

update_player_cpp()
fix_console_manager()
update_console_manager_h()
update_console_manager_cpp_return_type()
