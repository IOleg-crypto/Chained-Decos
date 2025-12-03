import re

def update_engine_h():
    file_path = r'core\engine\Engine.h'
    with open(file_path, 'r', encoding='utf-8') as f:
        content = f.read()

    # Replace forward declarations with includes
    includes = """#include "core/interfaces/IPlayer.h"
#include "core/interfaces/IMenu.h"
#include "core/interfaces/ILevelManager.h"
"""
    # Remove old forward declarations
    content = re.sub(r'class Player;\s*', '', content)
    content = re.sub(r'class Menu;\s*', '', content)
    content = re.sub(r'class LevelManager;\s*', '', content)
    
    # Add includes (if not present)
    if '#include "core/interfaces/IPlayer.h"' not in content:
        # Find a good place to insert includes. Maybe after other includes.
        # Engine.h usually has some includes at top.
        content = content.replace('#include <memory>', '#include <memory>\n' + includes)

    # Change return types
    content = content.replace('Player *GetPlayer() const;', 'IPlayer *GetPlayer() const;')
    content = content.replace('LevelManager *GetLevelManager() const;', 'ILevelManager *GetLevelManager() const;')
    content = content.replace('Menu *GetMenu() const;', 'IMenu *GetMenu() const;')
    
    with open(file_path, 'w', encoding='utf-8', newline='\r\n') as f:
        f.write(content)
    print("Updated Engine.h")

def update_engine_cpp():
    file_path = r'core\engine\Engine.cpp'
    with open(file_path, 'r', encoding='utf-8') as f:
        content = f.read()
        
    # Change return types in implementation
    content = content.replace('Player *Engine::GetPlayer() const', 'IPlayer *Engine::GetPlayer() const')
    content = content.replace('LevelManager *Engine::GetLevelManager() const', 'ILevelManager *Engine::GetLevelManager() const')
    content = content.replace('Menu *Engine::GetMenu() const', 'IMenu *Engine::GetMenu() const')
    
    with open(file_path, 'w', encoding='utf-8', newline='\r\n') as f:
        f.write(content)
    print("Updated Engine.cpp")

update_engine_h()
update_engine_cpp()
