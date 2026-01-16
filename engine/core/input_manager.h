#ifndef CH_INPUT_MANAGER_H
#define CH_INPUT_MANAGER_H

#include "input_context.h"
#include <memory>
#include <stack>
#include <string>
#include <unordered_map>

namespace CHEngine
{

// Central manager for Input Action System
// Manages contexts, actions, and bindings
class InputManager
{
public:
    static void Init();
    static void Shutdown();

    // Context management
    static void RegisterContext(const std::string &name);
    static InputContext *GetContext(const std::string &name);
    static void PushContext(const std::string &name);
    static void PopContext();
    static InputContext *GetActiveContext();

    // Quick action registration (uses active context)
    static void RegisterAction(const std::string &name, InputActionType type);
    static void BindKey(const std::string &actionName, int keyCode,
                        InputAxis axis = InputAxis::None, float scale = 1.0f);

    // Action subscription (for scripts)
    static void SubscribeToAction(const std::string &actionName, std::function<void()> callback);
    static void SubscribeToAction(const std::string &actionName,
                                  std::function<void(float)> callback);
    static void SubscribeToAction(const std::string &actionName,
                                  std::function<void(Vector2)> callback);

    // Load input graph from JSON
    static bool LoadInputGraph(const std::string &path);
    static bool SaveInputGraph(const std::string &path, const std::string &contextName);

    // Internal processing (called by Application/Input)
    static void ProcessKeyPressed(int keyCode);
    static void ProcessKeyReleased(int keyCode);
    static void ProcessAxisInput();

private:
    InputManager() = default;

    static std::unordered_map<std::string, std::shared_ptr<InputContext>> s_Contexts;
    static std::stack<std::string> s_ContextStack;
};

} // namespace CHEngine

#endif // CH_INPUT_MANAGER_H
