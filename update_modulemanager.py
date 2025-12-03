import re

file_path = r'core\object\module\Core\ModuleManager.cpp'
with open(file_path, 'r', encoding='utf-8') as f:
    content = f.read()

# Update RegisterModule
content = content.replace(
    '    IEngineModule *rawPtr = module.get();\n    m_modules.push_back(std::move(module));\n    m_moduleByName[moduleName] = rawPtr;',
    '    m_modules[moduleName] = std::move(module);\n    m_registrationOrder.push_back(moduleName);'
)

# Update GetModule
content = content.replace(
    '    auto it = m_moduleByName.find(name);\n    if (it != m_moduleByName.end())\n    {\n        return it->second;\n    }',
    '    auto it = m_modules.find(name);\n    if (it != m_modules.end())\n    {\n        return it->second.get();\n    }'
)

# Update IsModuleLoaded
content = content.replace(
    '    return m_moduleByName.find(name) != m_moduleByName.end();',
    '    return m_modules.find(name) != m_modules.end();'
)

# Update ShutdownAllModules
shutdown_pattern = r'for \(auto it = m_modules\.rbegin\(\); it != m_modules\.rend\(\); \+\+it\)\s*\{\s*if \(\*it && \(\*it\)->IsInitialized\(\)\)\s*\{\s*\(\*it\)->Shutdown\(\);\s*\}\s*\}'
shutdown_replacement = 'for (auto it = m_registrationOrder.rbegin(); it != m_registrationOrder.rend(); ++it)\n    {\n        auto modIt = m_modules.find(*it);\n        if (modIt != m_modules.end() && modIt->second && modIt->second->IsInitialized())\n        {\n            modIt->second->Shutdown();\n        }\n    }'
content = re.sub(shutdown_pattern, shutdown_replacement, content, flags=re.DOTALL)

# Update SortModulesByDependencies (iterating modules)
sort_pattern = r'for \(const auto &module : m_modules\)\s*\{\s*if \(module\)\s*\{\s*modules\.push_back\(module\.get\(\)\);\s*\}\s*\}'
sort_replacement = 'for (const auto &pair : m_modules)\n    {\n        if (pair.second)\n        {\n            modules.push_back(pair.second.get());\n        }\n    }'
content = re.sub(sort_pattern, sort_replacement, content, flags=re.DOTALL)

with open(file_path, 'w', encoding='utf-8', newline='\r\n') as f:
    f.write(content)

print("Updated ModuleManager.cpp")
