#ifndef CH_PROPERTY_EDITOR_H
#define CH_PROPERTY_EDITOR_H

#include <functional>
#include <string>

#include "engine/scene/component_registry.h"
#include "engine/scene/entity.h"

namespace Chained
{
class PropertyEditor
{
public:
    static void Init();

    // Registry API
    static void DrawEntityProperties(Entity entity);
    static void DrawAddComponentPopup(Entity entity);

    // Automation: Register using Reflection
    template <typename T> static void Register(const std::string& name, const char* icon = nullptr);

    // Custom Drawer Registration
    template <typename T>
    static void RegisterCustom(const std::string& name, std::function<bool(T&, Entity)> drawer,
                               const char* icon = nullptr);

    static void DrawEntityHeader(Chained::Entity entity);

private:
    static class EditorLayer* s_EditorLayer;

    // Internal template helpers (Implementations moved to .cpp or a separate _impl.h if needed elsewhere)
    template <typename T> static void DrawComponentReflection(const std::string& name, const char* icon, Entity entity);
    static void DrawGenericReflection(const ComponentMetadata& metadata, Entity entity);

    template <typename T>
    static void DrawComponentContainer(const std::string& name, const char* icon, Entity entity,
                                       std::function<bool(T&, Entity)> drawer);

    // Final non-template drawing core
    static void DrawComponentInternal(::entt::id_type typeId, const std::string& name, const char* icon, Entity entity,
                                      std::function<bool()> contentDrawer, std::function<void()> remover);
};

} // namespace Chained

#endif // CH_PROPERTY_EDITOR_H
