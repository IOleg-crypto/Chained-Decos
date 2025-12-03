import re

def fix_console_manager_cpp():
    file_path = r'project\chaineddecos\Menu\Console\ConsoleManager.cpp'
    with open(file_path, 'r', encoding='utf-8') as f:
        content = f.read()
        
    # Fix GetPlayer implementation to cast
    # auto player = Engine::Instance().GetPlayer(); -> auto player = static_cast<Player*>(Engine::Instance().GetPlayer());
    if 'static_cast<Player*>' not in content:
        content = content.replace(
            'auto player = Engine::Instance().GetPlayer();',
            'auto player = static_cast<Player*>(Engine::Instance().GetPlayer());'
        )
        
    with open(file_path, 'w', encoding='utf-8', newline='\r\n') as f:
        f.write(content)
    print("Fixed ConsoleManager.cpp")

def fix_uimanager_cpp():
    file_path = r'project\chaineddecos\Systems\UIController\UIManager.cpp'
    with open(file_path, 'r', encoding='utf-8') as f:
        content = f.read()
        
    # Fix GetPlayer calls
    # There might be multiple ways it's called.
    # Pattern: m_engine->GetPlayer()
    # I'll replace it with static_cast<Player*>(m_engine->GetPlayer())
    # But be careful not to double cast if I run script twice (though I check usually).
    
    # Simple replacement might work if usage is like: m_engine->GetPlayer()->...
    # But if it's assigned to auto, auto will be Player* after cast.
    
    # Replace m_engine->GetPlayer() with static_cast<Player*>(m_engine->GetPlayer())
    # But need to include Player.h if not included (it is included).
    
    # Use regex to avoid replacing inside static_cast if already there?
    # Or just simple replace and check.
    
    if 'static_cast<Player*>(m_engine->GetPlayer())' not in content:
        content = content.replace('m_engine->GetPlayer()', 'static_cast<Player*>(m_engine->GetPlayer())')
        
    # Fix GetLevelManager calls
    if 'static_cast<LevelManager*>(m_engine->GetLevelManager())' not in content:
        content = content.replace('m_engine->GetLevelManager()', 'static_cast<LevelManager*>(m_engine->GetLevelManager())')
        
    # Also check for Engine::Instance().GetPlayer() if used
    if 'static_cast<Player*>(Engine::Instance().GetPlayer())' not in content:
        content = content.replace('Engine::Instance().GetPlayer()', 'static_cast<Player*>(Engine::Instance().GetPlayer())')

    if 'static_cast<LevelManager*>(Engine::Instance().GetLevelManager())' not in content:
        content = content.replace('Engine::Instance().GetLevelManager()', 'static_cast<LevelManager*>(Engine::Instance().GetLevelManager())')

    with open(file_path, 'w', encoding='utf-8', newline='\r\n') as f:
        f.write(content)
    print("Fixed UIManager.cpp")

fix_console_manager_cpp()
fix_uimanager_cpp()
