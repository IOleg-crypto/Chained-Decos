import re

# Update Player.h to implement IPlayer
with open(r'project\chaineddecos\Player\Core\Player.h', 'r', encoding='utf-8') as f:
    content = f.read()

# Add IPlayer include
if '#include "core/interfaces/IPlayer.h"' not in content:
    # Add after existing includes, before forward declarations
    content = content.replace(
        '#include "servers/rendering/Interfaces/IGameRenderable.h"',
        '#include "servers/rendering/Interfaces/IGameRenderable.h"\n#include "core/interfaces/IPlayer.h"'
    )

# Change class declaration to inherit from IPlayer
content = re.sub(
    r'class Player : public IPlayerMediator',
    'class Player : public IPlayer, public IPlayerMediator',
    content
)

with open(r'project\chaineddecos\Player\Core\Player.h', 'w', encoding='utf-8', newline='\r\n') as f:
    f.write(content)

print("Player.h updated - now implements IPlayer interface")

# Update Menu.h to implement IMenu
with open(r'project\chaineddecos\Menu\Menu.h', 'r', encoding='utf-8') as f:
    content = f.read()

# Add IMenu include
if '#include "core/interfaces/IMenu.h"' not in content:
    content = content.replace(
        '#include <vector>',
        '#include <vector>\n#include "core/interfaces/IMenu.h"'
    )

# Change class declaration
content = re.sub(
    r'class Menu : public IMenuRenderable',
    'class Menu : public IMenu, public IMenuRenderable',
    content
)

with open(r'project\chaineddecos\Menu\Menu.h', 'w', encoding='utf-8', newline='\r\n') as f:
    f.write(content)

print("Menu.h updated - now implements IMenu interface")

# Update LevelManager.h to implement ILevelManager
with open(r'project\chaineddecos\Systems\MapSystem\LevelManager.h', 'r', encoding='utf-8') as f:
    content = f.read()

# Add ILevelManager include
if '#include "core/interfaces/ILevelManager.h"' not in content:
    content = content.replace(
        '#include <vector>',
        '#include <vector>\n#include "core/interfaces/ILevelManager.h"'
    )

# Change class declaration
content = re.sub(
    r'class LevelManager : public IEngineModule',
    'class LevelManager : public ILevelManager, public IEngineModule',
    content
)

with open(r'project\chaineddecos\Systems\MapSystem\LevelManager.h', 'w', encoding='utf-8', newline='\r\n') as f:
    f.write(content)

print("LevelManager.h updated - now implements ILevelManager interface")
print("\nAll concrete classes now implement their interfaces!")
