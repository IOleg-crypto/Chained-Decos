import re

def fix_levelmanager_cpp():
    file_path = r'project\chaineddecos\Systems\MapSystem\LevelManager.cpp'
    with open(file_path, 'r', encoding='utf-8') as f:
        content = f.read()
        
    # Fix assignment in Update
    # m_player = player; -> m_player = static_cast<Player*>(player);
    # But wait, 'player' variable itself is auto player = m_engine->GetPlayer();
    # So 'player' is IPlayer*.
    # I can change the assignment: m_player = static_cast<Player*>(player);
    
    if 'm_player = static_cast<Player*>(player);' not in content:
        content = content.replace('m_player = player;', 'm_player = static_cast<Player*>(player);')
        
    with open(file_path, 'w', encoding='utf-8', newline='\r\n') as f:
        f.write(content)
    print("Fixed LevelManager.cpp")

fix_levelmanager_cpp()
