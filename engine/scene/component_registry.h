#ifndef CH_COMPONENT_REGISTRY_H
#define CH_COMPONENT_REGISTRY_H

#include "engine/scene/entity.h"
#include "engine/scene/scene.h"
#include <entt/entt.hpp>
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
#include <yaml-cpp/yaml.h>

#include "engine/core/reflection.h"

namespace CHEngine
{
    /**
     * @brief Metadata required for a component to be integrated into Editor and Serialization.
     */
    struct ComponentMetadata
    {
        std::string Name;
        std::string SerializationKey;
        const char* Icon = nullptr;
        
        // UI Drawing callback (Inspector)
        std::function<void(Entity)> DrawUI;
        
        // Serialization callbacks
        std::function<void(YAML::Emitter&, Entity)> Serialize;
        std::function<void(Entity, YAML::Node)> Deserialize;
        
        // Lifecycle callbacks
        std::function<bool(Entity)> Has;
        std::function<std::vector<uint64_t>(class Scene*)> GetAll;
        std::function<void(Entity)> Add;
        std::function<void(Entity)> Remove;
        std::function<void(Entity, Entity)> Copy;
        
        // Visibility and filters
        std::string Category = "Other";
        bool Visible = true;
        bool AllowAdd = true;
        bool IsWidget = false;
        bool IsReflective = false;

        // Type-erased reflection caller. 
        // The void* is the archive pointer, and the second int is the reflection mode.
        // This allows the Editor or Serializer to pass their specific archives.
        std::function<void(Entity, void*, int)> ReflectInternal;
    };

    /**
     * @brief Centralized registry for all components (Engine & Game).
     */
    class ComponentRegistry
    {
    public:
        static void Register(entt::id_type typeId, const ComponentMetadata& metadata);
        static void RegisterEngineComponents();
        
        static const std::unordered_map<entt::id_type, ComponentMetadata>& GetRegistry() { return s_Registry; }
        static bool Exists(entt::id_type typeId) { return s_Registry.contains(typeId); }
        static const ComponentMetadata& GetMetadata(::entt::id_type typeId)
        {
            auto it = s_Registry.find(typeId);
            CH_CORE_ASSERT(it != s_Registry.end(), "ComponentRegistry::GetMetadata — type not registered!");
            return it->second;
        }
        static ComponentMetadata& GetMetadataMutable(::entt::id_type typeId)
        {
            auto it = s_Registry.find(typeId);
            CH_CORE_ASSERT(it != s_Registry.end(), "ComponentRegistry::GetMetadataMutable — type not registered!");
            return it->second;
        }

        static void SetAllowAdd(::entt::id_type typeId, bool allow) { s_Registry.at(typeId).AllowAdd = allow; }
        static void SetVisible(::entt::id_type typeId, bool visible) { s_Registry.at(typeId).Visible = visible; }
        static void SetIsWidget(::entt::id_type typeId, bool isWidget) { s_Registry.at(typeId).IsWidget = isWidget; }

        /**
         * @brief Registers a component that implements 'void Reflect(Properties& props)'.
         * Automatically generates UI and Serialization handlers.
         */
        template<typename T>
        static void RegisterReflective(const std::string& name, const char* icon = nullptr, const std::string& category = "Game")
        {
            ComponentMetadata metadata;
            metadata.Name = name;
            metadata.SerializationKey = name + "Component";
            metadata.Icon = icon;
            metadata.Category = category;
            
            metadata.Has = [](Entity e) { return e.HasComponent<T>(); };
            metadata.GetAll = [](class Scene* s) { 
                std::vector<uint64_t> ids;
                for (auto ent : s->GetRegistry().view<T>())
                {
                    ids.push_back((uint64_t)(uint32_t)ent);
                }
                return ids;
            };
            metadata.Add = [](Entity e) { if (!e.HasComponent<T>()) e.AddComponent<T>(); };
            metadata.Remove = [](Entity e) { if (e.HasComponent<T>()) e.RemoveComponent<T>(); };
            metadata.Copy = [](Entity src, Entity dst) { if (src.HasComponent<T>()) dst.AddOrReplaceComponent<T>(src.GetComponent<T>()); };

            metadata.IsReflective = true;
            metadata.ReflectInternal = [](Entity e, void* archivePtr, int mode) {
                IPropertyArchive* archive = static_cast<IPropertyArchive*>(archivePtr);
                const ReflectionMode reflMode = static_cast<ReflectionMode>(mode);

                if (reflMode == ReflectionMode::Deserialize)
                {
                    // Add component if it doesn't exist yet (it won't during scene loading)
                    auto& comp = e.AddOrReplaceComponent<T>();
                    GenericProperties props(*archive);
                    comp.Reflect(props);
                }
                else if (e.HasComponent<T>())
                {
                    GenericProperties props(*archive);
                    e.GetComponent<T>().Reflect(props);
                }
            };

            Register(entt::type_hash<T>::value(), metadata);
        }

    private:
        static std::unordered_map<entt::id_type, ComponentMetadata> s_Registry;
    };

    /**
     * @brief Macro for automatic static registration of components.
     * Use this in your component's .cpp file.
     */
    #define CH_REGISTER_COMPONENT(type, name, icon) \
        static bool s_ComponentRegistered_##type = []() { \
            ::CHEngine::ComponentRegistry::RegisterReflective<type>(name, icon); \
            return true; \
        }()

} // namespace CHEngine

#endif // CH_COMPONENT_REGISTRY_H
