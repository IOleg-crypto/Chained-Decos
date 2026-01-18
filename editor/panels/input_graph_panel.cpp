#include "input_graph_panel.h"
#include "engine/core/log.h"
#include <imgui.h>
#include <raylib.h>
#include <unordered_map>

namespace CHEngine
{

InputGraphPanel::InputGraphPanel()
{
    CH_CORE_INFO("InputGraphPanel initialized");
}

void InputGraphPanel::OnImGuiRender()
{
    ImGui::Begin("Input Graph Editor");

    if (ImGui::Button("Save"))
    {
        SaveCurrentActions();
    }
    ImGui::SameLine();
    if (ImGui::Button("Load"))
    {
        LoadActions("assets/input/gameplay_input.json");
    }

    ImGui::Separator();

    RenderActionList();

    // Modals
    if (m_ShowAddActionModal)
        ShowAddActionModal();
    if (m_ShowEditBindingModal)
        ShowEditBindingModal();
    if (m_ShowKeyDetectionModal)
        ShowKeyDetectionModal();

    ImGui::End();
}

void InputGraphPanel::RenderActionList()
{
    auto &actions = Input::GetActions();
    if (actions.empty())
    {
        ImGui::TextColored(ImVec4(1, 1, 0, 1), "No actions registered.");
        if (ImGui::Button("Load assets/input/gameplay_input.json"))
        {
            LoadActions("assets/input/gameplay_input.json");
        }
    }

    ImGui::Text("Actions:");
    ImGui::Separator();

    for (auto &[name, action] : actions)
    {
        RenderActionNode(name, &action);
    }

    ImGui::Spacing();
    if (ImGui::Button("+ Add Action"))
    {
        m_ShowAddActionModal = true;
        ImGui::OpenPopup("Add New Action");
    }
}

void InputGraphPanel::RenderActionNode(const std::string &actionName, InputAction *action)
{
    if (!action)
        return;

    ImVec4 headerColor;
    switch (action->Type)
    {
    case InputActionType::Button:
        headerColor = ImVec4(0.2f, 0.8f, 0.2f, 1.0f);
        break;
    case InputActionType::Axis1D:
        headerColor = ImVec4(0.2f, 0.5f, 0.8f, 1.0f);
        break;
    case InputActionType::Axis2D:
        headerColor = ImVec4(0.8f, 0.5f, 0.2f, 1.0f);
        break;
    }

    ImGui::PushStyleColor(ImGuiCol_Header, headerColor);
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(headerColor.x * 1.2f, headerColor.y * 1.2f,
                                                         headerColor.z * 1.2f, 1.0f));

    std::string label = actionName + " (" + ActionTypeToString(action->Type) + ")";
    if (ImGui::CollapsingHeader(label.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Indent();

        ImGui::Text("Bindings:");
        auto &bindings = action->Bindings;
        for (size_t i = 0; i < bindings.size(); ++i)
        {
            RenderBindingRow(actionName, bindings[i], static_cast<int>(i));
        }

        if (ImGui::Button(("+ Add Binding##" + actionName).c_str()))
        {
            OnAddBinding(actionName);
        }

        ImGui::SameLine();
        if (ImGui::Button(("- Remove Action##" + actionName).c_str()))
        {
            Input::RemoveAction(actionName);
        }

        ImGui::Unindent();
    }

    ImGui::PopStyleColor(2);
    ImGui::Spacing();
}

void InputGraphPanel::RenderBindingRow(const std::string &actionName, const InputBinding &binding,
                                       int index)
{
    ImGui::PushID(index);

    std::string bindingText = "• " + KeyCodeToString(binding.KeyCode);

    if (binding.Axis != InputAxis::None)
    {
        bindingText += " → " + std::string(AxisToString(binding.Axis)) + ": ";
        bindingText += (binding.Scale >= 0 ? "+" : "") + std::to_string(binding.Scale);
    }

    ImGui::Text("%s", bindingText.c_str());
    ImGui::SameLine();

    if (ImGui::SmallButton("Edit"))
    {
        OnEditBinding(actionName, index);
    }
    ImGui::SameLine();

    if (ImGui::SmallButton("Remove"))
    {
        OnRemoveBinding(actionName, index);
    }

    ImGui::PopID();
}

void InputGraphPanel::ShowAddActionModal()
{
    if (ImGui::BeginPopupModal("Add New Action", &m_ShowAddActionModal))
    {
        ImGui::InputText("Name", m_NewActionName, 256);

        const char *types[] = {"Button", "Axis1D", "Axis2D"};
        ImGui::Combo("Type", &m_NewActionTypeIndex, types, 3);

        if (ImGui::Button("Create"))
        {
            Input::RegisterAction(m_NewActionName, 0,
                                  static_cast<InputActionType>(m_NewActionTypeIndex));
            m_ShowAddActionModal = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
        {
            m_ShowAddActionModal = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

void InputGraphPanel::ShowEditBindingModal()
{
    if (ImGui::BeginPopupModal("Edit Binding", &m_ShowEditBindingModal))
    {
        ImGui::Text("Action: %s", m_EditingActionName.c_str());
        ImGui::Separator();

        if (ImGui::Button("Detect Key"))
        {
            m_ShowKeyDetectionModal = true;
            ImGui::OpenPopup("Detect Key");
        }

        if (m_DetectedKey != 0)
        {
            ImGui::Text("Key: %s", KeyCodeToString(m_DetectedKey).c_str());
        }

        ImGui::Checkbox("Require Shift", &m_EditingBinding.RequireShift);
        ImGui::Checkbox("Require Ctrl", &m_EditingBinding.RequireCtrl);
        ImGui::Checkbox("Require Alt", &m_EditingBinding.RequireAlt);

        auto &actions = Input::GetActions();
        auto it = actions.find(m_EditingActionName);
        if (it != actions.end() && it->second.Type != InputActionType::Button)
        {
            ImGui::Separator();
            const char *axes[] = {"None", "X", "Y"};
            ImGui::Combo("Axis", &m_AxisIndex, axes, 3);
            ImGui::InputFloat("Scale", &m_AxisScale);
        }

        if (ImGui::Button("OK"))
        {
            ApplyBinding();
            m_ShowEditBindingModal = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
        {
            m_ShowEditBindingModal = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

void InputGraphPanel::ShowKeyDetectionModal()
{
    if (ImGui::BeginPopupModal("Detect Key", &m_ShowKeyDetectionModal,
                               ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Press any key...");
        ImGui::Spacing();

        int key = GetKeyPressed();
        if (key != 0)
        {
            m_DetectedKey = key;
            m_EditingBinding.KeyCode = key;
        }

        if (m_DetectedKey != 0)
        {
            ImGui::TextColored(ImVec4(0, 1, 0, 1), "Detected: %s",
                               KeyCodeToString(m_DetectedKey).c_str());
        }

        ImGui::Spacing();
        if (ImGui::Button("OK") || (m_DetectedKey != 0 && ImGui::IsKeyPressed(ImGuiKey_Enter)))
        {
            m_ShowKeyDetectionModal = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
        {
            m_DetectedKey = 0;
            m_ShowKeyDetectionModal = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

void InputGraphPanel::SaveCurrentActions()
{
    if (Input::SaveActions("assets/input/gameplay_input.json"))
    {
        CH_CORE_INFO("Saved input actions to assets/input/gameplay_input.json");
    }
    else
    {
        CH_CORE_ERROR("Failed to save input actions");
    }
}

void InputGraphPanel::LoadActions(const std::string &path)
{
    if (Input::LoadActions(path))
    {
        CH_CORE_INFO("Loaded input actions: {}", path);
    }
    else
    {
        CH_CORE_ERROR("Failed to load input actions: {}", path);
    }
}

void InputGraphPanel::OnAddBinding(const std::string &actionName)
{
    m_EditingActionName = actionName;
    m_EditingBinding = InputBinding{};
    m_EditingBindingIndex = -1;
    m_DetectedKey = 0;
    m_AxisIndex = 0;
    m_AxisScale = 1.0f;

    m_ShowEditBindingModal = true;
    ImGui::OpenPopup("Edit Binding");
}

void InputGraphPanel::OnEditBinding(const std::string &actionName, int bindingIndex)
{
    auto &actions = Input::GetActions();
    auto it = actions.find(actionName);
    if (it == actions.end())
        return;

    auto &bindings = it->second.Bindings;
    if (bindingIndex < 0 || bindingIndex >= static_cast<int>(bindings.size()))
        return;

    m_EditingActionName = actionName;
    m_EditingBinding = bindings[bindingIndex];
    m_EditingBindingIndex = bindingIndex;
    m_DetectedKey = m_EditingBinding.KeyCode;

    m_AxisIndex = static_cast<int>(m_EditingBinding.Axis);
    m_AxisScale = m_EditingBinding.Scale;

    m_ShowEditBindingModal = true;
    ImGui::OpenPopup("Edit Binding");
}

void InputGraphPanel::OnRemoveBinding(const std::string &actionName, int bindingIndex)
{
    auto &actions = Input::GetActions();
    auto it = actions.find(actionName);
    if (it == actions.end())
        return;

    auto &bindings = it->second.Bindings;
    if (bindingIndex < 0 || bindingIndex >= static_cast<int>(bindings.size()))
        return;

    Input::RemoveBinding(actionName, bindings[bindingIndex].KeyCode);
}

void InputGraphPanel::ApplyBinding()
{
    m_EditingBinding.Axis = static_cast<InputAxis>(m_AxisIndex);
    m_EditingBinding.Scale = m_AxisScale;

    auto &actions = Input::GetActions();
    auto it = actions.find(m_EditingActionName);
    if (it == actions.end())
        return;

    if (m_EditingBindingIndex >= 0)
    {
        if (m_EditingBindingIndex < (int)it->second.Bindings.size())
        {
            it->second.Bindings[m_EditingBindingIndex] = m_EditingBinding;
        }
    }
    else
    {
        Input::AddBinding(m_EditingActionName, m_EditingBinding);
    }
}

std::string InputGraphPanel::KeyCodeToString(int keyCode) const
{
    static const std::unordered_map<int, std::string> keyNames = {
        {KEY_W, "W"},
        {KEY_A, "A"},
        {KEY_S, "S"},
        {KEY_D, "D"},
        {KEY_SPACE, "SPACE"},
        {KEY_LEFT_SHIFT, "LEFT SHIFT"},
        {KEY_RIGHT_SHIFT, "RIGHT SHIFT"},
        {KEY_LEFT_CONTROL, "LEFT CTRL"},
        {KEY_RIGHT_CONTROL, "RIGHT CTRL"},
        {KEY_LEFT_ALT, "LEFT ALT"},
        {KEY_RIGHT_ALT, "RIGHT ALT"},
        {KEY_E, "E"},
        {KEY_R, "R"},
        {KEY_F, "F"},
        {KEY_Q, "Q"},
        {KEY_ESCAPE, "ESCAPE"},
        {KEY_ENTER, "ENTER"},
        {KEY_TAB, "TAB"},
        {KEY_BACKSPACE, "BACKSPACE"},
    };

    auto it = keyNames.find(keyCode);
    return (it != keyNames.end()) ? it->second : "KEY_" + std::to_string(keyCode);
}

std::string InputGraphPanel::ActionTypeToString(InputActionType type) const
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
        return "Unknown";
    }
}

const char *InputGraphPanel::AxisToString(InputAxis axis) const
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
}

} // namespace CHEngine
