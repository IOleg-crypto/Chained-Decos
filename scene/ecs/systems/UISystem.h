#ifndef UI_SYSTEM_H
#define UI_SYSTEM_H

#include <entt/entt.hpp>
#include <functional>
#include <string>
#include <unordered_map>

namespace CHEngine
{

// System for processing UI events in runtime
class UISystem
{
public:
    UISystem();
    ~UISystem() = default;

    // Process UI events for the current frame
    void ProcessUIEvents(entt::registry &registry);

    // Register event handler for button
    using ButtonCallback = std::function<void()>;
    void RegisterButtonHandler(const std::string &eventId, ButtonCallback callback);
    void UnregisterButtonHandler(const std::string &eventId);

private:
    void ProcessButtons(entt::registry &registry);
    bool IsButtonHovered(const struct RectTransform &transform);
    bool IsButtonClicked();

    std::unordered_map<std::string, ButtonCallback> m_ButtonHandlers;
};

} // namespace CHEngine

#endif // UI_SYSTEM_H
