#ifndef CH_UI_PROPERTIES_H
#define CH_UI_PROPERTIES_H

#include "editor_gui.h"
#include "engine/core/reflection.h"
#include "imgui/IconsFontAwesome6.h"

namespace CHEngine
{
// Implementation of the Archive concept for ImGui UI
class UIProperties
{
public:
    UIProperties() = default;

    template <typename T> bool Property(const char* name, T& value)
    {
        if constexpr (is_variant_v<T>)
        {
            return std::visit([&](auto&& v) { return EditorGUI::Property(name, v); }, value);
        }
        else if constexpr (std::is_same_v<T, float[4]>)
        {
            bool changed = EditorGUI::Property(name, *(glm::vec4*)&value);
            m_Changed |= changed;
            return changed;
        }
        else
        {
            bool changed = EditorGUI::Property(name, value);
            m_Changed |= changed;
            return changed;
        }
    }

    // Overload for enums
    bool Property(const char* name, int& value, const char** names, int count)
    {
        bool changed = EditorGUI::Property(name, value, names, count);
        m_Changed |= changed;
        return changed;
    }

    bool Handle(const char* name, uint64_t& value)
    {
        bool changed = EditorGUI::Property(name, value);
        m_Changed |= changed;
        return changed;
    }

    bool File(const char* name, std::string& path, const char* extensions = nullptr)
    {
        bool changed = EditorGUI::FileProperty(name, path, extensions);
        m_Changed |= changed;
        return changed;
    }

    void Action(const char* label, std::function<void()> func)
    {
        if (EditorGUI::ActionButton(nullptr, label))
        {
            func();
        }
    }

    template <typename T> bool Sequence(const char* name, std::vector<T>& values)
    {
        bool localChanged = false;
        if (ImGui::TreeNodeEx(name, ImGuiTreeNodeFlags_DefaultOpen))
        {
            for (size_t i = 0; i < (int)values.size(); i++)
            {
                ImGui::PushID((int)i);

                if (ImGui::Button(ICON_FA_TRASH))
                {
                    values.erase(values.begin() + i);
                    m_Changed = true;
                    localChanged = true;
                    ImGui::PopID();
                    break;
                }
                
                ImGui::SameLine();
                
                if constexpr (requires(T t, Properties<UIProperties>& p) { t.Reflect(p); })
                {
                    char label[32];
                    sprintf(label, "Item %d", (int)i);
                    if (ImGui::TreeNodeEx(label, ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth))
                    {
                        Properties<UIProperties> itemProps(*this);
                        values[i].Reflect(itemProps);
                        ImGui::TreePop();
                    }
                }
                else if constexpr (std::is_same_v<T, std::string>)
                {
                    char buf[256];
                    strncpy(buf, values[i].c_str(), sizeof(buf) - 1);
                    ImGui::SetNextItemWidth(-1);
                    if (ImGui::InputText("##val", buf, sizeof(buf)))
                    {
                        values[i] = buf;
                        m_Changed = true;
                        localChanged = true;
                    }
                }
                else
                {
                    ImGui::Text("Item %d", (int)i);
                }

                ImGui::PopID();
                if constexpr (!requires(T t, Properties<UIProperties>& p) { t.Reflect(p); }) 
                    ImGui::Separator();
            }

            ImGui::Spacing();
            if (ImGui::Button(ICON_FA_PLUS " Add New Item", ImVec2(-1, 0)))
            {
                values.push_back({});
                m_Changed = true;
                localChanged = true;
            }

            ImGui::TreePop();
        }
        return localChanged;
    }

    template <typename T> bool Nested(const char* name, T& value)
    {
        bool localChanged = false;
        if (ImGui::TreeNodeEx(name, ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen))
        {
            Properties<UIProperties> itemProps(*this);
            value.Reflect(itemProps);
            ImGui::TreePop();
        }
        return localChanged;
    }

    void Header(const char* label)
    {
        ImGui::Spacing();
        ImGui::TextColored({0.2f, 0.7f, 0.9f, 1.0f}, "%s", label);
        ImGui::Separator();
    }

    void Separator()
    {
        ImGui::Separator();
    }

    bool BeginGroup(const char* label, bool defaultOpen = true)
    {
        ImGuiTreeNodeFlags flags =
            ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_AllowOverlap | ImGuiTreeNodeFlags_FramePadding;
        if (defaultOpen)
        {
            flags |= ImGuiTreeNodeFlags_DefaultOpen;
        }

        bool opened = ImGui::TreeNodeEx(label, flags);
        return opened;
    }

    void EndGroup()
    {
        ImGui::TreePop();
    }

    bool HasChanged() const
    {
        return m_Changed;
    }
    void SetChanged(bool changed)
    {
        m_Changed = changed;
    }
    ReflectionMode GetReflectionMode() const
    {
        return ReflectionMode::UI;
    }

private:
    bool m_Changed = false;
};
} // namespace CHEngine

#endif // CH_UI_PROPERTIES_H
