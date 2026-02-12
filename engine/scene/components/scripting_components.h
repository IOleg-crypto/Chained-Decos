#ifndef CH_SCRIPTING_COMPONENTS_H
#define CH_SCRIPTING_COMPONENTS_H

#include <string>
#include <vector>

namespace CHEngine
{
class ScriptableEntity;

// Component that enables Native (C++) scripting for an entity.
    struct ScriptInstance
    {
        ScriptableEntity *Instance = nullptr;
        std::string ScriptName;

        ScriptableEntity *(*InstantiateScript)() = nullptr;
        void (*DestroyScript)(ScriptInstance *) = nullptr;

        ScriptInstance() = default;
        
        // Copying a ScriptInstance should NOT copy the active pointer!
        // The new component should instantiate its own script.
        ScriptInstance(const ScriptInstance& other)
            : ScriptName(other.ScriptName), InstantiateScript(other.InstantiateScript), DestroyScript(other.DestroyScript)
        {
            Instance = nullptr; 
        }

        ScriptInstance& operator=(const ScriptInstance& other)
        {
            if (this != &other)
            {
                ScriptName = other.ScriptName;
                InstantiateScript = other.InstantiateScript;
                DestroyScript = other.DestroyScript;
                Instance = nullptr;
            }
            return *this;
        }

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

        NativeScriptComponent() = default;
        NativeScriptComponent(const NativeScriptComponent&) = default; // Uses ScriptInstance copy ctor

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
