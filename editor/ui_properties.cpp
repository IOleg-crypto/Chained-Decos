#include "ui_properties.h"
#include <algorithm>
#include <cstring>

namespace Chained
{

bool UIProperties::EnumPropertyInternal(const char* name, int& value, const char** names, int count,
                                        const PropertyMeta& meta)
{
    ImGui::BeginDisabled(meta.ReadOnly);
    bool changed = EditorGUI::Property(name, value, names, count);
    ImGui::EndDisabled();
    if (!meta.Tooltip.empty() && ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("%s", meta.Tooltip.c_str());
    }
    UpdateState(changed);
    return changed;
}

bool UIProperties::StringEnumInternal(const char* name, std::string& value, const std::vector<std::string>& options,
                                      const PropertyMeta& meta)
{
    int currentIndex = 0;
    for (size_t i = 0; i < options.size(); ++i)
    {
        if (options[i] == value)
        {
            currentIndex = (int)i;
            break;
        }
    }

    std::vector<const char*> optionNames;
    for (const auto& option : options)
    {
        optionNames.push_back(option.c_str());
    }

    bool changed = false;
    if (ImGui::GetCurrentTable() != nullptr)
    {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::AlignTextToFramePadding();
        ImGui::Text("%s", name);
        ImGui::TableSetColumnIndex(1);
    }
    ImGui::PushID(name);
    ImGui::BeginDisabled(meta.ReadOnly);
    ImGui::SetNextItemWidth(-1);
    if (ImGui::Combo("##prop", &currentIndex, optionNames.data(), (int)optionNames.size()))
    {
        value = (currentIndex >= 0 && currentIndex < (int)options.size()) ? options[currentIndex] : std::string();
        changed = true;
    }
    ImGui::EndDisabled();
    ImGui::PopID();

    if (!meta.Tooltip.empty() && ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("%s", meta.Tooltip.c_str());
    }
    UpdateState(changed);
    return changed;
}

bool UIProperties::HandleInternal(const char* name, uint64_t& value)
{
    bool changed = EditorGUI::Property(name, value);
    UpdateState(changed);
    return changed;
}

bool UIProperties::FileInternal(const char* name, std::string& path, const char* extensions, const PropertyMeta& meta)
{
    ImGui::BeginDisabled(meta.ReadOnly);
    bool changed = EditorGUI::FileProperty(name, path, extensions);
    ImGui::EndDisabled();
    if (!meta.Tooltip.empty() && ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("%s", meta.Tooltip.c_str());
    }
    UpdateState(changed);
    return changed;
}

bool UIProperties::ActionInternal(const char* label, std::function<void()> func)
{
    if (EditorGUI::ActionButton(nullptr, label))
    {
        func();
        m_Changed = true;
        return true;
    }
    return false;
}

void UIProperties::HeaderInternal(const char* label)
{
    if (ImGui::GetCurrentTable() != nullptr)
    {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::AlignTextToFramePadding();
    }
    ImGui::Spacing();
    ImGui::TextColored({0.2f, 0.7f, 0.9f, 1.0f}, "%s", label);
    if (ImGui::GetCurrentTable() != nullptr)
    {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
    }
}

void UIProperties::SeparatorInternal()
{
    ImGui::Separator();
}

bool UIProperties::BeginGroupInternal(const char* label, bool defaultOpen)
{
    if (ImGui::GetCurrentTable() != nullptr)
    {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::AlignTextToFramePadding();
    }
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_AllowOverlap |
                               ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_SpanAvailWidth |
                               ImGuiTreeNodeFlags_SpanAllColumns;
    if (defaultOpen)
    {
        flags |= ImGuiTreeNodeFlags_DefaultOpen;
    }
    bool opened = ImGui::TreeNodeEx(label, flags);
    if (ImGui::GetCurrentTable() != nullptr)
    {
        ImGui::TableSetColumnIndex(1);
    }
    return opened;
}

void UIProperties::EndGroupInternal()
{
    ImGui::TreePop();
}

void UIProperties::UpdateState(bool changed)
{
    if (changed)
    {
        m_Changed = true;
    }
    if (ImGui::IsItemActivated())
    {
        m_Started = true;
    }
    if (ImGui::IsItemDeactivatedAfterEdit())
    {
        m_Finished = true;
    }
}

bool UIProperties::StringProperty(const char* name, std::string& value, const PropertyMeta& meta)
{
    return EditorGUI::Property(name, value);
}

} // namespace Chained
