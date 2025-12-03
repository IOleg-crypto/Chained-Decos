import re

def fix_menu_h():
    file_path = r'project\chaineddecos\Menu\Menu.h'
    with open(file_path, 'r', encoding='utf-8') as f:
        content = f.read()

    # Remove the block added by implement_interfaces.py
    # It starts with "// IMenu Interface Implementation"
    content = re.sub(r'// IMenu Interface Implementation.*?bool IsConsoleOpen\(\) const override \{ return IsConsoleOpen\(\); \} // Already exists\s*', '', content, flags=re.DOTALL)
    
    # Add proper overrides and new methods
    # Existing: ToggleConsole, IsConsoleOpen, Update, Render
    content = content.replace('void ToggleConsole();', 'void ToggleConsole() override;')
    content = content.replace('[[nodiscard]] bool IsConsoleOpen() const;', 'bool IsConsoleOpen() const override;')
    content = content.replace('void Update() override;', 'void Update() override;') # Already override
    content = content.replace('void Render() override;', 'void Render() override;') # Already override
    
    # Add new methods
    new_methods = """
    // IMenu Interface Implementation
    bool IsOpen() const override { return m_state != MenuState::GameMode; }
    void Show() override { m_state = MenuState::Main; }
    void Hide() override { m_state = MenuState::GameMode; }
    bool ShouldStartGame() const override { return m_action == MenuAction::StartGame || m_action == MenuAction::ResumeGame; }
    std::string GetSelectedMap() const override { return GetSelectedMapName(); }
    """
    
    if '// IMenu Interface Implementation' not in content:
        content = content.replace('// IMenuRenderable interface implementations', new_methods + '\n    // IMenuRenderable interface implementations')
        
    with open(file_path, 'w', encoding='utf-8', newline='\r\n') as f:
        f.write(content)
    print("Fixed Menu.h")

def fix_levelmanager_h():
    file_path = r'project\chaineddecos\Systems\MapSystem\LevelManager.h'
    with open(file_path, 'r', encoding='utf-8') as f:
        content = f.read()

    # Remove the block added by implement_interfaces.py
    content = re.sub(r'// ILevelManager Interface Implementation.*?Vector3 GetSpawnPosition\(\) const override \{ return GetPlayerSpawnPosition\(\); \}\s*', '', content, flags=re.DOTALL)
    
    # Add proper overrides and new methods
    # Existing: Update, Render
    # New: LoadMap, UnloadMap, IsMapLoaded, GetCurrentMapName, GetSpawnPosition
    
    new_methods = """
    // ILevelManager Interface Implementation
    bool LoadMap(const std::string& path) override { LoadEditorMap(path); return true; }
    void UnloadMap() override { /* TODO: Implement unload */ }
    bool IsMapLoaded() const override { return HasSpawnZone(); } // Use HasSpawnZone as proxy? Or m_gameMap != nullptr
    std::string GetCurrentMapName() const override { return GetCurrentMapPath(); }
    Vector3 GetSpawnPosition() const override { return GetPlayerSpawnPosition(); }
    """
    
    if '// ILevelManager Interface Implementation' not in content:
        content = content.replace('// IEngineModule interface', new_methods + '\n    // IEngineModule interface')
        
    with open(file_path, 'w', encoding='utf-8', newline='\r\n') as f:
        f.write(content)
    print("Fixed LevelManager.h")

fix_menu_h()
fix_levelmanager_h()
