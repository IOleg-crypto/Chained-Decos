import re

file_path = r'core\object\module\Core\ModuleManager.cpp'
with open(file_path, 'r', encoding='utf-8') as f:
    content = f.read()

# Fix SortModulesByDependencies iteration
# Old code might look like: for (const auto &module : m_modules)
# New code needs: for (const auto &pair : m_modules) { auto& module = pair.second; ... }

# Find the SortModulesByDependencies function body
start_marker = "std::vector<IEngineModule *> ModuleManager::SortModulesByDependencies() const"
end_marker = "return sorted;"

if start_marker in content:
    # We need to replace the loop inside this function
    # The previous python script might have done a partial replace, let's fix it properly
    
    # Replace the loop that iterates m_modules
    content = content.replace(
        "for (const auto &module : m_modules)",
        "for (const auto &pair : m_modules)"
    )
    
    # Inside the loop, 'module' is now 'pair' (if we just changed the variable name in loop)
    # But the body uses 'module'. So let's add a reference
    content = content.replace(
        "for (const auto &pair : m_modules)\n    {",
        "for (const auto &pair : m_modules)\n    {\n        const auto &module = pair.second;"
    )
    
    # Also fix the visit lambda if it captures module
    # Actually, let's look at the specific error: visit(module.get())
    # If module is unique_ptr, module.get() is correct.
    # But if we iterate map, we get pair.
    
    pass

# Let's rewrite the SortModulesByDependencies function completely to be safe
new_sort_func = """std::vector<IEngineModule *> ModuleManager::SortModulesByDependencies() const
{
    std::vector<IEngineModule *> sorted;
    std::set<std::string> visited;
    std::set<std::string> visiting;

    std::function<void(IEngineModule *)> visit = [&](IEngineModule *module) {
        if (!module)
            return;

        std::string name = module->GetModuleName();
        if (visited.find(name) != visited.end())
            return;

        if (visiting.find(name) != visiting.end())
        {
            TraceLog(LOG_WARNING, "[ModuleManager] Circular dependency detected involving module: %s",
                     name.c_str());
            return;
        }

        visiting.insert(name);

        for (const auto &depName : module->GetDependencies())
        {
            if (m_modules.find(depName) != m_modules.end())
            {
                visit(m_modules.at(depName).get());
            }
        }

        visiting.erase(name);
        visited.insert(name);
        sorted.push_back(module);
    };

    for (const auto &pair : m_modules)
    {
        if (pair.second)
        {
            visit(pair.second.get());
        }
    }

    return sorted;
}"""

# Regex to replace the whole function
content = re.sub(
    r'std::vector<IEngineModule \*> ModuleManager::SortModulesByDependencies\(\) const\s*\{.*?\n\}',
    new_sort_func,
    content,
    flags=re.DOTALL
)

# Also fix GetLoadedModules
new_get_loaded = """std::vector<std::string> ModuleManager::GetLoadedModules() const
{
    std::vector<std::string> names;
    names.reserve(m_modules.size());
    for (const auto &pair : m_modules)
    {
        if (pair.second)
        {
            names.push_back(pair.second->GetModuleName());
        }
    }
    return names;
}"""

content = re.sub(
    r'std::vector<std::string> ModuleManager::GetLoadedModules\(\) const\s*\{.*?\n\}',
    new_get_loaded,
    content,
    flags=re.DOTALL
)

# Fix UpdateAllModules
new_update = """void ModuleManager::UpdateAllModules(float deltaTime)
{
    if (!m_initialized)
    {
        return;
    }

    for (const auto &name : m_registrationOrder)
    {
        auto it = m_modules.find(name);
        if (it != m_modules.end() && it->second && it->second->IsInitialized())
        {
            it->second->Update(deltaTime);
        }
    }
}"""

content = re.sub(
    r'void ModuleManager::UpdateAllModules\(float deltaTime\)\s*\{.*?\n\}',
    new_update,
    content,
    flags=re.DOTALL
)

# Fix RenderAllModules
new_render = """void ModuleManager::RenderAllModules()
{
    if (!m_initialized)
    {
        return;
    }

    for (const auto &name : m_registrationOrder)
    {
        auto it = m_modules.find(name);
        if (it != m_modules.end() && it->second && it->second->IsInitialized())
        {
            it->second->Render();
        }
    }
}"""

content = re.sub(
    r'void ModuleManager::RenderAllModules\(\)\s*\{.*?\n\}',
    new_render,
    content,
    flags=re.DOTALL
)

with open(file_path, 'w', encoding='utf-8', newline='\r\n') as f:
    f.write(content)

print("Fixed ModuleManager.cpp iteration logic")
