#ifndef UI_SYSTEM_H
#define UI_SYSTEM_H

#include <entt/entt.hpp>
#include <functional>
#include <raylib.h>
#include <string>
#include <unordered_map>

namespace CHEngine
{

// System for processing UI events in runtime
class UISystem
{
public:
    // Initialize the system
    UISystem();
    ~UISystem() = default;

    // Process and Render UI for a specific scene
    void Render(entt::registry &registry, int screenWidth, int screenHeight,
                Vector2 offset = {0, 0});

    // Register event handler for button (C++/Script bridge)
    using ButtonCallback = std::function<void()>;
    void RegisterButtonHandler(const std::string &eventId, ButtonCallback callback);
    void UnregisterButtonHandler(const std::string &eventId);

private:
    void RenderEntity(entt::registry &registry, entt::entity entity, int screenWidth,
                      int screenHeight, Vector2 offset);

    // Helpers
    Vector2 CalculateScreenPosition(const struct RectTransform &transform, int screenWidth,
                                    int screenHeight);

    std::unordered_map<std::string, ButtonCallback> m_ButtonHandlers;
};

} // namespace CHEngine

#endif // UI_SYSTEM_H
