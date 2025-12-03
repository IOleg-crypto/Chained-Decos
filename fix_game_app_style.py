import re

# Read the file
with open(r'project\chaineddecos\GameApplication.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

# Add Menu::SetupStyle() call after RenderManager initialization
pattern = r'(// Initialize RenderManager with config\s+RenderManager::Get\(\)\.Initialize\(m_gameConfig\.width, m_gameConfig\.height, "Chained Decos"\);)'
replacement = r'\1\n\n    // Setup ImGui style for menu (after ImGui is initialized by RenderManager)\n    auto* engine = &Engine::Instance();\n    if (engine->GetModuleManager())\n    {\n        auto* uiModule = engine->GetModuleManager()->GetModule("UI");\n        if (uiModule)\n        {\n            UIManager* uiManager = dynamic_cast<UIManager*>(uiModule);\n            if (uiManager && uiManager->GetMenu())\n            {\n                uiManager->GetMenu()->SetupStyle();\n                TraceLog(LOG_INFO, "[GameApplication] ImGui style configured");\n            }\n        }\n    }'

content = re.sub(pattern, replacement, content)

# Write back
with open(r'project\chaineddecos\GameApplication.cpp', 'w', encoding='utf-8', newline='\r\n') as f:
    f.write(content)

print("GameApplication.cpp fixed - SetupStyle() call added after RenderManager!")
