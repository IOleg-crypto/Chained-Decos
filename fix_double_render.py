import re

file_path = r'project\chaineddecos\GameApplication.cpp'
with open(file_path, 'r', encoding='utf-8') as f:
    content = f.read()

# Remove BeginFrame() call from OnRender
content = content.replace('    // Begin Frame\n    RenderManager::Get().BeginFrame();', '    // Frame is begun by EngineApplication::Render()')
content = content.replace('    // Begin Frame\r\n    RenderManager::Get().BeginFrame();', '    // Frame is begun by EngineApplication::Render()')

# Remove EndFrame() call from OnRender
content = content.replace('    // End Frame\n    RenderManager::Get().EndFrame();', '    // Frame is ended by EngineApplication::Render()')
content = content.replace('    // End Frame\r\n    RenderManager::Get().EndFrame();', '    // Frame is ended by EngineApplication::Render()')

with open(file_path, 'w', encoding='utf-8', newline='\r\n') as f:
    f.write(content)

print("Removed duplicate BeginFrame/EndFrame from GameApplication::OnRender()")
