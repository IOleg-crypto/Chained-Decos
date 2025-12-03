import re

# Update Engine.h to use interfaces
with open(r'core\engine\Engine.h', 'r', encoding='utf-8') as f:
    content = f.read()

# Replace forward declarations with interface includes
old_forwards = '''// Forward declarations for game objects
class Player;
class LevelManager;
class Menu;
class RenderManager;
class InputManager;'''

new_includes = '''// Interface includes - no forward declarations needed
#include "core/interfaces/IPlayer.h"
#include "core/interfaces/ILevelManager.h"
#include "core/interfaces/IMenu.h"

// Forward declarations for singletons (acceptable for now)
class RenderManager;
class InputManager;'''

content = content.replace(old_forwards, new_includes)

# Replace return types in method declarations
content = re.sub(r'Player \*GetPlayer\(\)', 'IPlayer *GetPlayer()', content)
content = re.sub(r'LevelManager \*GetLevelManager\(\)', 'ILevelManager *GetLevelManager()', content)
content = re.sub(r'Menu \*GetMenu\(\)', 'IMenu *GetMenu()', content)

with open(r'core\engine\Engine.h', 'w', encoding='utf-8', newline='\r\n') as f:
    f.write(content)

print("Engine.h updated - using interfaces instead of forward declarations")

# Update Engine.cpp to cast to interfaces
with open(r'core\engine\Engine.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

# Update return types
content = re.sub(r'Player \*Engine::GetPlayer\(\)', 'IPlayer *Engine::GetPlayer()', content)
content = re.sub(r'LevelManager \*Engine::GetLevelManager\(\)', 'ILevelManager *Engine::GetLevelManager()', content)
content = re.sub(r'Menu \*Engine::GetMenu\(\)', 'IMenu *Engine::GetMenu()', content)

with open(r'core\engine\Engine.cpp', 'w', encoding='utf-8', newline='\r\n') as f:
    f.write(content)

print("Engine.cpp updated - return types changed to interfaces")
