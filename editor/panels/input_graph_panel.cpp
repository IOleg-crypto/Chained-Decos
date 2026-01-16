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

    // Top bar with Save/Load buttons
    if (ImGui::Button("Save"))
    {
        SaveCurrentContext();
    }
    ImGui::SameLine();
    if (ImGui::Button("Load"))
    {
        // TODO: File dialog
        LoadContext("assets/input/gameplay_input.json");
    }

    ImGui::Separator();

    RenderContextSelector();
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

void InputGraphPanel::RenderContextSelector()
{
    ImGui::Text("Context:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(200);

    // For now, just display current context
    ImGui::Text("%s", m_SelectedContext.c_str());
}

void InputGraphPanel::RenderActionList()
{
    auto *context = InputManager::GetContext(m_SelectedContext);
    if (!context)
    {
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "Context '%s' not found!",
                           m_SelectedContext.c_str());

        if (ImGui::Button("Create Default Gameplay Context"))
        {
            InputManager::RegisterContext("Gameplay");
            CH_CORE_INFO("Created default Gameplay context via editor");
        }
        ImGui::SameLine();
        if (ImGui::Button("Load assets/input/gameplay_input.json"))
        {
            LoadContext("assets/input/gameplay_input.json");
        }
        return;
    }

    ImGui::Text("Actions:");
    ImGui::Separator();

    const auto &actions = context->GetActions();
    for (const auto &[name, action] : actions)
    {
        RenderActionNode(name, action.get());
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

    // Color based on type
    ImVec4 headerColor;
    switch (action->GetType())
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

    std::string label = actionName + " (" + ActionTypeToString(action->GetType()) + ")";
    if (ImGui::CollapsingHeader(label.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Indent();

        auto *context = InputManager::GetContext(m_SelectedContext);
        auto bindings = context->GetBindingsForAction(actionName);

        ImGui::Text("Bindings:");
        for (size_t i = 0; i < bindings.size(); ++i)
        {
            RenderBindingRow(actionName, bindings[i], static_cast<int>(i));
        }

        if (ImGui::Button(("+ Add Binding##" + actionName).c_str()))
        {
            OnAddBinding(actionName);
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

    // Display binding info
    std::string bindingText = "• " + KeyCodeToString(binding.KeyCode);

    if (binding.Axis != InputAxis::None)
    {
        bindingText += " → " + std::string(AxisToString(binding.Axis)) + ": ";
        bindingText += (binding.Scale >= 0 ? "+" : "") + std::to_string(binding.Scale);
    }

    ImGui::Text("%s", bindingText.c_str());
    ImGui::SameLine();

    // Edit button
    if (ImGui::SmallButton("Edit"))
    {
        OnEditBinding(actionName, index);
    }
    ImGui::SameLine();

    // Remove button
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
            auto *context = InputManager::GetContext(m_SelectedContext);
            if (context)
            {
                InputActionType type = static_cast<InputActionType>(m_NewActionTypeIndex);
                context->RegisterAction(m_NewActionName, type);
                CH_CORE_INFO("Created action: {}", m_NewActionName);
            }
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

        // Modifiers
        ImGui::Checkbox("Require Shift", &m_EditingBinding.RequireShift);
        ImGui::Checkbox("Require Ctrl", &m_EditingBinding.RequireCtrl);
        ImGui::Checkbox("Require Alt", &m_EditingBinding.RequireAlt);

        // Axis settings (for Axis2D actions)
        auto *context = InputManager::GetContext(m_SelectedContext);
        auto *action = context ? context->GetAction(m_EditingActionName) : nullptr;
        if (action && action->GetType() == InputActionType::Axis2D)
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

        // Detect key press
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

void InputGraphPanel::SaveCurrentContext()
{
    std::string path = "assets/input/" + m_SelectedContext + "_input.json";
    if (InputManager::SaveInputGraph(path, m_SelectedContext))
    {
        CH_CORE_INFO("Saved input graph: {}", path);
    }
    else
    {
        CH_CORE_ERROR("Failed to save input graph");
    }
}

void InputGraphPanel::LoadContext(const std::string &path)
{
    if (InputManager::LoadInputGraph(path))
    {
        CH_CORE_INFO("Loaded input graph: {}", path);
    }
    else
    {
        CH_CORE_ERROR("Failed to load input graph: {}", path);
    }
}

void InputGraphPanel::OnAddBinding(const std::string &actionName)
{
    m_EditingActionName = actionName;
    m_EditingBinding = InputBinding{};
    m_EditingBinding.ActionName = actionName;
    m_EditingBindingIndex = -1;
    m_DetectedKey = 0;
    m_AxisIndex = 0;
    m_AxisScale = 1.0f;

    m_ShowEditBindingModal = true;
    ImGui::OpenPopup("Edit Binding");
}

void InputGraphPanel::OnEditBinding(const std::string &actionName, int bindingIndex)
{
    auto *context = InputManager::GetContext(m_SelectedContext);
    if (!context)
        return;

    auto bindings = context->GetBindingsForAction(actionName);
    if (bindingIndex < 0 || bindingIndex >= static_cast<int>(bindings.size()))
        return;

    m_EditingActionName = actionName;
    m_EditingBinding = bindings[bindingIndex];
    m_EditingBindingIndex = bindingIndex;
    m_DetectedKey = m_EditingBinding.KeyCode;

    // Set axis settings
    m_AxisIndex = static_cast<int>(m_EditingBinding.Axis);
    m_AxisScale = m_EditingBinding.Scale;

    m_ShowEditBindingModal = true;
    ImGui::OpenPopup("Edit Binding");
}

void InputGraphPanel::OnRemoveBinding(const std::string &actionName, int bindingIndex)
{
    auto *context = InputManager::GetContext(m_SelectedContext);
    if (!context)
        return;

    auto bindings = context->GetBindingsForAction(actionName);
    if (bindingIndex < 0 || bindingIndex >= static_cast<int>(bindings.size()))
        return;

    context->RemoveBinding(actionName, bindings[bindingIndex].KeyCode);
    CH_CORE_INFO("Removed binding from action: {}", actionName);
}

void InputGraphPanel::ApplyBinding()
{
    auto *context = InputManager::GetContext(m_SelectedContext);
    if (!context)
        return;

    // Set axis from UI
    m_EditingBinding.Axis = static_cast<InputAxis>(m_AxisIndex);
    m_EditingBinding.Scale = m_AxisScale;

    // If editing existing binding, remove old one first
    if (m_EditingBindingIndex >= 0)
    {
        auto bindings = context->GetBindingsForAction(m_EditingActionName);
        if (m_EditingBindingIndex < static_cast<int>(bindings.size()))
        {
            context->RemoveBinding(m_EditingActionName, bindings[m_EditingBindingIndex].KeyCode);
        }
    }

    // Add new/updated binding
    context->AddBinding(m_EditingBinding);
    CH_CORE_INFO("Applied binding to action: {}", m_EditingActionName);
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
