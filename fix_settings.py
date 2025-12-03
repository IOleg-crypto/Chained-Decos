import re

# Read the file
with open(r'project\chaineddecos\Menu\Settings\SettingsManager.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

# Define the pattern to match and remove
pattern = r'(\s+}\s*\n\s*\n)\s+// Apply loaded settings to the game\s+int width, height;\s+m_config\.GetResolution\(width, height\);\s+SetWindowSize\(width, height\);\s+\n\s+if \(m_config\.IsFullscreen\(\)\) \{\s+SetWindowState\(FLAG_FULLSCREEN_MODE\);\s+\}\s+\n\s+if \(m_config\.IsVSync\(\)\) \{\s+SetWindowState\(FLAG_VSYNC_HINT\);\s+\}\s+\n'

replacement = r'\1\n    // NOTE: We do NOT apply window settings here (SetWindowSize, SetWindowState)\n    // because this is called from the constructor, before the window exists.\n    // Window settings will be applied by GameApplication::OnConfigure() and OnStart()\n\n'

# Replace
content = re.sub(pattern, replacement, content, flags=re.MULTILINE)

# Write back
with open(r'project\chaineddecos\Menu\Settings\SettingsManager.cpp', 'w', encoding='utf-8', newline='\r\n') as f:
    f.write(content)

print("File fixed successfully!")
