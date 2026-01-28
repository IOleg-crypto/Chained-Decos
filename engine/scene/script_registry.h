#ifndef CH_SCRIPT_REGISTRY_H
#define CH_SCRIPT_REGISTRY_H

#include "components/scripting_components.h"
#include "functional"
#include "string"
#include "unordered_map"

namespace CHEngine
{
void RegisterGameScripts(); // Implemented by the game

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

    template <typename T> static void Register(const std::string &name)
    {
        s_Registry[name] = {[]() { return static_cast<ScriptableEntity *>(new T()); },
                            [](ScriptInstance *si)
                            {
                                delete si->Instance;
                                si->Instance = nullptr;
                            }};
    }

    static void AddScript(const std::string &name, NativeScriptComponent &nsc)
    {
        if (s_Registry.find(name) != s_Registry.end())
        {
            ScriptInstance si;
            si.ScriptName = name;
            si.InstantiateScript = s_Registry[name].Instantiate;
            si.DestroyScript = s_Registry[name].Destroy;
            nsc.Scripts.push_back(si);
        }
    }

    static const std::unordered_map<std::string, ScriptFunctions> &GetScripts()
    {
        return s_Registry;
    }

private:
    inline static std::unordered_map<std::string, ScriptFunctions> s_Registry;
};
} // namespace CHEngine

#endif // CH_SCRIPT_REGISTRY_H
