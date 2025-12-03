import re

file_path = r'project\chaineddecos\GameApplication.cpp'
with open(file_path, 'r', encoding='utf-8') as f:
    content = f.read()

# Add OnRender implementation before the last closing brace
on_render = """
void GameApplication::OnRender()
{
    // Rendering is handled by modules (UIManager, LevelManager)
    // through moduleManager->RenderAllModules() in EngineApplication::Render()
    // No additional rendering needed here for now
}
"""

# Insert before the last line (which should be empty or just whitespace)
if 'void GameApplication::OnRender()' not in content:
    content = content.rstrip() + '\n' + on_render + '\n'

with open(file_path, 'w', encoding='utf-8', newline='\r\n') as f:
    f.write(content)

print("Added OnRender() to GameApplication.cpp")
