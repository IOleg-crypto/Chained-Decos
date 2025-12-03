import re

# Fix Player.h - remove inline SetAudioManager
with open(r'project\chaineddecos\Player\Core\Player.h', 'r', encoding='utf-8') as f:
    content = f.read()

# Replace inline SetAudioManager with declaration only
pattern = r'    // Service injection\r?\n    void SetAudioManager\(std::shared_ptr<AudioManager> audioManager\)\r?\n    \{\r?\n        m_audioManager = audioManager;\r?\n    \}'
replacement = '    // Service injection\n    void SetAudioManager(std::shared_ptr<AudioManager> audioManager);'

content = re.sub(pattern, replacement, content)

with open(r'project\chaineddecos\Player\Core\Player.h', 'w', encoding='utf-8', newline='\r\n') as f:
    f.write(content)

print("Player.h fixed - removed inline SetAudioManager")

# Fix Menu.h - remove inline GetSettingsManager  
with open(r'project\chaineddecos\Menu\Menu.h', 'r', encoding='utf-8') as f:
    content = f.read()

pattern = r'    // Settings manager access\r?\n    \[\[nodiscard\]\] SettingsManager \*GetSettingsManager\(\) const\r?\n    \{\r?\n        return m_settingsManager\.get\(\);\r?\n    \}'
replacement = '    // Settings manager access\n    [[nodiscard]] SettingsManager *GetSettingsManager() const;'

content = re.sub(pattern, replacement, content)

with open(r'project\chaineddecos\Menu\Menu.h', 'w', encoding='utf-8', newline='\r\n') as f:
    f.write(content)

print("Menu.h fixed - removed inline GetSettingsManager")

# Add GetSettingsManager to Menu.cpp
with open(r'project\chaineddecos\Menu\Menu.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

# Find the end of SetupStyle function and add GetSettingsManager after it
pattern = r'(    colors\[ImGuiCol_HeaderActive\] = ImVec4\(0\.35f, 0\.35f, 0\.35f, 1\.0f\);\r?\n\})'
replacement = r'\1\n\nSettingsManager *Menu::GetSettingsManager() const\n{\n    return m_settingsManager.get();\n}'

if 'SettingsManager *Menu::GetSettingsManager()' not in content:
    content = re.sub(pattern, replacement, content)
    print("Menu.cpp - added GetSettingsManager implementation")
else:
    print("Menu.cpp - GetSettingsManager already exists")

with open(r'project\chaineddecos\Menu\Menu.cpp', 'w', encoding='utf-8', newline='\r\n') as f:
    f.write(content)

print("All fixes applied!")
