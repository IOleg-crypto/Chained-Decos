#ifndef CH_SCRIPTING_COMPONENTS_H
#define CH_SCRIPTING_COMPONENTS_H

#include <string>
#include <vector>

namespace CHEngine
{
class ScriptableEntity;

/**
 * Component that enables Native (C++) scripting for an entity.
 */
struct ScriptInstance
{
    ScriptableEntity *Instance = nullptr;
    std::string ScriptName;

    ScriptableEntity *(*InstantiateScript)() = nullptr;
    void (*DestroyScript)(ScriptInstance *) = nullptr;

    ScriptInstance() = default;

    template <typename T> void Bind(const std::string &name)
    {
        ScriptName = name;
        InstantiateScript = []() { return static_cast<ScriptableEntity *>(new T()); };
        DestroyScript = [](ScriptInstance *si)
        {
            delete si->Instance;
            si->Instance = nullptr;
        };
    }
};

struct NativeScriptComponent
{
    std::vector<ScriptInstance> Scripts;

    ~NativeScriptComponent()
    {
        for (auto &script : Scripts)
        {
            if (script.Instance && script.DestroyScript)
            {
                script.DestroyScript(&script);
            }
        }
    }
};

} // namespace CHEngine

#endif // CH_SCRIPTING_COMPONENTS_H
