#ifndef CH_COMPONENT_UI_H
#define CH_COMPONENT_UI_H

#include "engine/scene/entity.h"

#include <unordered_map>

namespace CHEngine
{
class ComponentUI
{
public:
    static void Init();

    // Registry-based API
    static void RegisterDrawer(entt::id_type typeId, std::function<void(Entity)> drawer);
    static void DrawEntityComponents(Entity entity);

    template <typename T> static void Register(std::function<void(Entity)> drawer)
    {
        RegisterDrawer(entt::type_hash<T>::value(), drawer);
    }

    static void DrawTag(Entity entity);
    static void DrawAddComponentPopup(Entity entity);

    static void DrawTransform(Entity entity);
    static void DrawModel(Entity entity);
    static void DrawMaterial(Entity entity, int hitMeshIndex = -1);
    static void DrawCollider(Entity entity);
    static void DrawRigidBody(Entity entity);
    static void DrawSpawn(Entity entity);
    static void DrawPlayer(Entity entity);
    static void DrawPointLight(Entity entity);
    static void DrawAudio(Entity entity);
    static void DrawHierarchy(Entity entity);
    static void DrawNativeScript(Entity entity);
    static void DrawAnimation(Entity entity);

    // Helpers (Internal Use)
    static void DrawTextStyle(struct TextStyle &style);
    static void DrawUIStyle(struct UIStyle &style);

    // UI Widgets
    static void DrawWidget(Entity entity);
    static void DrawImageWidget(Entity entity);
    static void DrawTextWidget(Entity entity);
    static void DrawButtonWidget(Entity entity);
    static void DrawSliderWidget(Entity entity);
    static void DrawCheckboxWidget(Entity entity);

private:
    static std::unordered_map<entt::id_type, std::function<void(Entity)>> s_DrawerRegistry;
};
} // namespace CHEngine

#endif // CH_COMPONENT_UI_H
