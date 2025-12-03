import re

def fix_imenu_h():
    file_path = r'core\interfaces\IMenu.h'
    with open(file_path, 'r', encoding='utf-8') as f:
        content = f.read()
    
    # Rename GetSelectedMap to GetSelectedMapName to avoid conflict with existing method returning optional<MapInfo>
    content = content.replace('virtual std::string GetSelectedMap() const = 0;', 'virtual std::string GetSelectedMapName() const = 0;')
    
    with open(file_path, 'w', encoding='utf-8', newline='\r\n') as f:
        f.write(content)
    print("Fixed IMenu.h (renamed GetSelectedMap -> GetSelectedMapName)")

def fix_menu_h():
    file_path = r'project\chaineddecos\Menu\Menu.h'
    with open(file_path, 'r', encoding='utf-8') as f:
        content = f.read()
        
    # Fix inheritance if missing
    if 'public IMenu,' not in content and 'public IMenuRenderable' in content:
        content = content.replace('class Menu : public IMenuRenderable', 'class Menu : public IMenu, public IMenuRenderable')
        
    # Remove the conflicting override I added
    content = content.replace('std::string GetSelectedMap() const override { return GetSelectedMapName(); }', '')
    
    # Mark existing GetSelectedMapName as override
    content = content.replace('[[nodiscard]] std::string GetSelectedMapName() const;', 'std::string GetSelectedMapName() const override;')
    
    with open(file_path, 'w', encoding='utf-8', newline='\r\n') as f:
        f.write(content)
    print("Fixed Menu.h inheritance and signatures")

def fix_levelmanager_h():
    file_path = r'project\chaineddecos\Systems\MapSystem\LevelManager.h'
    with open(file_path, 'r', encoding='utf-8') as f:
        content = f.read()
        
    # Fix inheritance if missing
    if 'public ILevelManager,' not in content and 'public IEngineModule' in content:
        content = content.replace('class LevelManager : public IEngineModule', 'class LevelManager : public ILevelManager, public IEngineModule')
        
    with open(file_path, 'w', encoding='utf-8', newline='\r\n') as f:
        f.write(content)
    print("Fixed LevelManager.h inheritance")

fix_imenu_h()
fix_menu_h()
fix_levelmanager_h()
