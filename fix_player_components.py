import re

def update_player_input_h():
    """Update PlayerInput to use IPlayer"""
    file_path = r'project\chaineddecos\Player\Components\PlayerInput.h'
    with open(file_path, 'r', encoding='utf-8') as f:
        content = f.read()
    
    content = content.replace('#include "../Interfaces/IPlayerMediator.h"', '#include "core/interfaces/IPlayer.h"')
    content = content.replace('IPlayerMediator *player', 'IPlayer *player')
    content = content.replace('IPlayerMediator* m_player', 'IPlayer* m_player')
    
    with open(file_path, 'w', encoding='utf-8', newline='\r\n') as f:
        f.write(content)
    print("Updated PlayerInput.h")

def update_player_input_cpp():
    """Update PlayerInput.cpp"""
    file_path = r'project\chaineddecos\Player\Components\PlayerInput.cpp'
    with open(file_path, 'r', encoding='utf-8') as f:
        content = f.read()
    
    content = content.replace('IPlayerMediator *player', 'IPlayer *player')
    content = content.replace('IPlayerMediator* player', 'IPlayer* player')
    
    with open(file_path, 'w', encoding='utf-8', newline='\r\n') as f:
        f.write(content)
    print("Updated PlayerInput.cpp")

def update_player_collision_cpp():
    """Update PlayerCollision.cpp"""
    file_path = r'project\chaineddecos\Player\Collision\PlayerCollision.cpp'
    with open(file_path, 'r', encoding='utf-8') as f:
        content = f.read()
    
    content = content.replace('IPlayerMediator* player', 'IPlayer* player')
    content = content.replace('IPlayerMediator *player', 'IPlayer *player')
    
    with open(file_path, 'w', encoding='utf-8', newline='\r\n') as f:
        f.write(content)
    print("Updated PlayerCollision.cpp")

def update_player_movement_h():
    """Update PlayerMovement to use IPlayer"""
    file_path = r'project\chaineddecos\Player\Components\PlayerMovement.h'
    with open(file_path, 'r', encoding='utf-8') as f:
        content = f.read()
    
    if 'IPlayerMediator' in content:
        content = content.replace('#include "../Interfaces/IPlayerMediator.h"', '#include "core/interfaces/IPlayer.h"')
        content = content.replace('IPlayerMediator *player', 'IPlayer *player')
        content = content.replace('IPlayerMediator* m_player', 'IPlayer* m_player')
        
        with open(file_path, 'w', encoding='utf-8', newline='\r\n') as f:
            f.write(content)
        print("Updated PlayerMovement.h")
    else:
        print("PlayerMovement.h - no changes needed")

def update_player_movement_cpp():
    """Update PlayerMovement.cpp"""
    file_path = r'project\chaineddecos\Player\Components\PlayerMovement.cpp'
    with open(file_path, 'r', encoding='utf-8') as f:
        content = f.read()
    
    if 'IPlayerMediator' in content:
        content = content.replace('IPlayerMediator *player', 'IPlayer *player')
        content = content.replace('IPlayerMediator* player', 'IPlayer* player')
        
        with open(file_path, 'w', encoding='utf-8', newline='\r\n') as f:
            f.write(content)
        print("Updated PlayerMovement.cpp")
    else:
        print("PlayerMovement.cpp - no changes needed")

# Run all updates
update_player_input_h()
update_player_input_cpp()
update_player_collision_cpp()
update_player_movement_h()
update_player_movement_cpp()

print("\nAll component files updated.")
