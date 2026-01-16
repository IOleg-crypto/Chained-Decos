#include "input_manager.h"
#include "engine/core/log.h"
#include <fstream>
#include <nlohmann/json.hpp>
#include <raylib.h>

using json = nlohmann::json;

namespace CHEngine
{

std::unordered_map<std::string, std::shared_ptr<InputContext>> InputManager::s_Contexts;
std::stack<std::string> InputManager::s_ContextStack;

// Helper: Convert string to KeyCode
static int StringToKeyCode(const std::string &keyName)
{
    static const std::unordered_map<std::string, int> keyMap = {
        {"KEY_W", KEY_W},
        {"KEY_A", KEY_A},
        {"KEY_S", KEY_S},
        {"KEY_D", KEY_D},
        {"KEY_SPACE", KEY_SPACE},
        {"KEY_LEFT_SHIFT", KEY_LEFT_SHIFT},
        {"KEY_LEFT_CONTROL", KEY_LEFT_CONTROL},
        {"KEY_LEFT_ALT", KEY_LEFT_ALT},
        {"KEY_E", KEY_E},
        {"KEY_R", KEY_R},
        {"KEY_F", KEY_F},
        {"KEY_ESCAPE", KEY_ESCAPE},
        {"KEY_ENTER", KEY_ENTER},
        {"KEY_TAB", KEY_TAB},
        {"KEY_BACKSPACE", KEY_BACKSPACE},
        // Add more as needed
    };

    auto it = keyMap.find(keyName);
    return (it != keyMap.end()) ? it->second : KEY_NULL;
}

// Helper: Convert string to InputAxis
static InputAxis StringToAxis(const std::string &axisName)
{
    if (axisName == "X")
        return InputAxis::X;
    if (axisName == "Y")
        return InputAxis::Y;
    return InputAxis::None;
}

void InputManager::Init()
{
    CH_CORE_INFO("InputManager initialized");
}

void InputManager::Shutdown()
{
    s_Contexts.clear();
    while (!s_ContextStack.empty())
        s_ContextStack.pop();

    CH_CORE_INFO("InputManager shut down");
}

void InputManager::RegisterContext(const std::string &name)
{
    if (s_Contexts.find(name) != s_Contexts.end())
    {
        CH_CORE_WARN("Context '{0}' already registered", name);
        return;
    }

    s_Contexts[name] = std::make_shared<InputContext>(name);
    CH_CORE_INFO("Registered input context: {0}", name);
}

InputContext *InputManager::GetContext(const std::string &name)
{
    auto it = s_Contexts.find(name);
    if (it != s_Contexts.end())
    {
        return it->second.get();
    }
    return nullptr;
}

void InputManager::PushContext(const std::string &name)
{
    if (s_Contexts.find(name) == s_Contexts.end())
    {
        CH_CORE_ERROR("Cannot push context '{0}': not registered", name);
        return;
    }

    s_ContextStack.push(name);
    CH_CORE_INFO("Pushed input context: {0}", name);
}

void InputManager::PopContext()
{
    if (!s_ContextStack.empty())
    {
        std::string popped = s_ContextStack.top();
        s_ContextStack.pop();
        CH_CORE_INFO("Popped input context: {0}", popped);
    }
    else
    {
        CH_CORE_WARN("Cannot pop context: stack is empty");
    }
}

InputContext *InputManager::GetActiveContext()
{
    if (!s_ContextStack.empty())
    {
        return GetContext(s_ContextStack.top());
    }
    return nullptr;
}

void InputManager::RegisterAction(const std::string &name, InputActionType type)
{
    auto *context = GetActiveContext();
    if (context)
    {
        context->RegisterAction(name, type);
    }
    else
    {
        CH_CORE_ERROR("Cannot register action '{0}': no active context", name);
    }
}

void InputManager::BindKey(const std::string &actionName, int keyCode, InputAxis axis, float scale)
{
    auto *context = GetActiveContext();
    if (context)
    {
        InputBinding binding;
        binding.ActionName = actionName;
        binding.KeyCode = keyCode;
        binding.Axis = axis;
        binding.Scale = scale;
        context->AddBinding(binding);
    }
    else
    {
        CH_CORE_ERROR("Cannot bind key: no active context");
    }
}

void InputManager::SubscribeToAction(const std::string &actionName, std::function<void()> callback)
{
    auto *context = GetActiveContext();
    if (context)
    {
        auto *action = context->GetAction(actionName);
        if (action)
        {
            action->SubscribePressed(callback);
        }
        else
        {
            CH_CORE_ERROR("Cannot subscribe: action '{0}' not found", actionName);
        }
    }
}

void InputManager::SubscribeToAction(const std::string &actionName,
                                     std::function<void(float)> callback)
{
    auto *context = GetActiveContext();
    if (context)
    {
        auto *action = context->GetAction(actionName);
        if (action)
        {
            action->SubscribeAxis1D(callback);
        }
        else
        {
            CH_CORE_ERROR("Cannot subscribe: action '{0}' not found", actionName);
        }
    }
}

void InputManager::SubscribeToAction(const std::string &actionName,
                                     std::function<void(Vector2)> callback)
{
    auto *context = GetActiveContext();
    if (context)
    {
        auto *action = context->GetAction(actionName);
        if (action)
        {
            action->SubscribeAxis2D(callback);
        }
        else
        {
            CH_CORE_ERROR("Cannot subscribe: action '{0}' not found", actionName);
        }
    }
}

bool InputManager::LoadInputGraph(const std::string &path)
{
    std::ifstream file(path);
    if (!file.is_open())
    {
        CH_CORE_ERROR("Failed to open input graph: {0}", path);
        return false;
    }

    try
    {
        json j;
        file >> j;

        std::string contextName = j["name"];
        RegisterContext(contextName);
        auto *context = GetContext(contextName);

        // Load actions
        for (const auto &actionJson : j["actions"])
        {
            std::string name = actionJson["name"];
            std::string typeStr = actionJson["type"];

            InputActionType type = InputActionType::Button;
            if (typeStr == "Axis1D")
                type = InputActionType::Axis1D;
            else if (typeStr == "Axis2D")
                type = InputActionType::Axis2D;

            context->RegisterAction(name, type);

            // Load bindings for this action
            for (const auto &bindingJson : actionJson["bindings"])
            {
                InputBinding binding;
                binding.ActionName = name;
                binding.KeyCode = StringToKeyCode(bindingJson["key"]);

                if (bindingJson.contains("axis"))
                {
                    binding.Axis = StringToAxis(bindingJson["axis"]);
                }
                if (bindingJson.contains("scale"))
                {
                    binding.Scale = bindingJson["scale"];
                }

                context->AddBinding(binding);
            }
        }

        CH_CORE_INFO("Loaded input graph: {0}", path);
        return true;
    }
    catch (const std::exception &e)
    {
        CH_CORE_ERROR("Failed to parse input graph: {0}", e.what());
        return false;
    }
}

bool InputManager::SaveInputGraph(const std::string &path, const std::string &contextName)
{
    auto *context = GetContext(contextName);
    if (!context)
    {
        CH_CORE_ERROR("Cannot save: context '{0}' not found", contextName);
        return false;
    }

    try
    {
        json j;
        j["name"] = contextName;
        j["version"] = "1.0";
        j["actions"] = json::array();

        // Helper: KeyCode to string
        auto KeyCodeToString = [](int keyCode) -> std::string
        {
            static const std::unordered_map<int, std::string> keyMap = {
                {KEY_W, "KEY_W"},
                {KEY_A, "KEY_A"},
                {KEY_S, "KEY_S"},
                {KEY_D, "KEY_D"},
                {KEY_SPACE, "KEY_SPACE"},
                {KEY_LEFT_SHIFT, "KEY_LEFT_SHIFT"},
                {KEY_LEFT_CONTROL, "KEY_LEFT_CONTROL"},
                {KEY_LEFT_ALT, "KEY_LEFT_ALT"},
                {KEY_E, "KEY_E"},
                {KEY_R, "KEY_R"},
                {KEY_F, "KEY_F"},
                {KEY_ESCAPE, "KEY_ESCAPE"},
                {KEY_ENTER, "KEY_ENTER"},
                {KEY_TAB, "KEY_TAB"},
                {KEY_BACKSPACE, "KEY_BACKSPACE"},
            };
            auto it = keyMap.find(keyCode);
            return (it != keyMap.end()) ? it->second : "KEY_" + std::to_string(keyCode);
        };

        // Helper: ActionType to string
        auto ActionTypeToString = [](InputActionType type) -> std::string
        {
            switch (type)
            {
            case InputActionType::Button:
                return "Button";
            case InputActionType::Axis1D:
                return "Axis1D";
            case InputActionType::Axis2D:
                return "Axis2D";
            default:
                return "Button";
            }
        };

        // Helper: Axis to string
        auto AxisToString = [](InputAxis axis) -> std::string
        {
            switch (axis)
            {
            case InputAxis::X:
                return "X";
            case InputAxis::Y:
                return "Y";
            default:
                return "None";
            }
        };

        // Serialize all actions
        for (const auto &[name, action] : context->GetActions())
        {
            json actionJson;
            actionJson["name"] = name;
            actionJson["type"] = ActionTypeToString(action->GetType());
            actionJson["bindings"] = json::array();

            // Serialize bindings for this action
            for (const auto &binding : context->GetBindingsForAction(name))
            {
                json bindingJson;
                bindingJson["key"] = KeyCodeToString(binding.KeyCode);

                if (binding.Axis != InputAxis::None)
                {
                    bindingJson["axis"] = AxisToString(binding.Axis);
                    bindingJson["scale"] = binding.Scale;
                }

                actionJson["bindings"].push_back(bindingJson);
            }

            j["actions"].push_back(actionJson);
        }

        // Write to file
        std::ofstream file(path);
        if (!file.is_open())
        {
            CH_CORE_ERROR("Failed to open file for writing: {0}", path);
            return false;
        }

        file << j.dump(2); // Pretty print with 2-space indent
        file.close();

        CH_CORE_INFO("Saved input graph: {0}", path);
        return true;
    }
    catch (const std::exception &e)
    {
        CH_CORE_ERROR("Failed to save input graph: {0}", e.what());
        return false;
    }
}

void InputManager::ProcessKeyPressed(int keyCode)
{
    auto *context = GetActiveContext();
    if (context)
    {
        context->ProcessKeyPressed(keyCode);
    }
}

void InputManager::ProcessKeyReleased(int keyCode)
{
    auto *context = GetActiveContext();
    if (context)
    {
        context->ProcessKeyReleased(keyCode);
    }
}

void InputManager::ProcessAxisInput()
{
    auto *context = GetActiveContext();
    if (context)
    {
        context->ProcessAxisInput();
    }
}

} // namespace CHEngine
