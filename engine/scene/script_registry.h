#ifndef CH_SCRIPT_REGISTRY_H
#define CH_SCRIPT_REGISTRY_H

#include "components/scripting_components.h"
#include <functional>
#include <string>
#include <unordered_map>

namespace CHEngine
{
class ScriptRegistry
{
public:
    using InstantiateFn = ScriptableEntity *(*)();
    using DestroyFn = void (*)(ScriptInstance *);

    struct ScriptFunctions
    {
        InstantiateFn Instantiate;
        DestroyFn Destroy;
    };

    template <typename T> void Register(const std::string &name)
    {
        m_Registry[name] = {[]() { return static_cast<ScriptableEntity *>(new T()); },
                            [](ScriptInstance *si)
                            {
                                delete si->Instance;
                                si->Instance = nullptr;
                            }};
    }

    void AddScript(const std::string &name, NativeScriptComponent &nsc)
    {
        if (m_Registry.find(name) != m_Registry.end())
        {
            ScriptInstance si;
            si.ScriptName = name;
            si.InstantiateScript = m_Registry[name].Instantiate;
            si.DestroyScript = m_Registry[name].Destroy;
            nsc.Scripts.push_back(si);
        }
    }

    const std::unordered_map<std::string, ScriptFunctions> &GetScripts() const
    {
        return m_Registry;
    }

    void CopyFrom(const ScriptRegistry& other)
    {
        m_Registry = other.m_Registry;
    }

    static ScriptRegistry& GetGlobalRegistry()
    {
        static ScriptRegistry s_GlobalRegistry;
        return s_GlobalRegistry;
    }

private:
    std::unordered_map<std::string, ScriptFunctions> m_Registry;
};
} // namespace CHEngine

#endif // CH_SCRIPT_REGISTRY_H
