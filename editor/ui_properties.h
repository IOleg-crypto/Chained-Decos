#pragma once

#include "engine/core/reflection.h"
#include "editor_gui.h"
#include "imgui/IconsFontAwesome6.h"

namespace CHEngine
{
    // Implementation of the Archive concept for ImGui UI
    class UIProperties
    {
    public:
        UIProperties() = default;

        template<typename T>
        bool Property(const char* name, T& value)
        {
            if constexpr (is_variant_v<T>)
            {
                return std::visit([&](auto&& v) {
                    return EditorGUI::Property(name, v);
                }, value);
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
            bool changed = EditorGUI::Property(name, path, extensions);
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

        template<typename T>
        bool Sequence(const char* name, std::vector<T>& values)
        {
            bool localChanged = false;
            if (ImGui::TreeNodeEx(name, ImGuiTreeNodeFlags_DefaultOpen))
            {
                for (size_t i = 0; i < values.size(); i++)
                {
                    ImGui::PushID((int)i);
                    
                    // Draw a remove button before each item
                    if (ImGui::Button(ICON_FA_TRASH))
                    {
                        values.erase(values.begin() + i);
                        m_Changed = true;
                        localChanged = true;
                        ImGui::PopID();
                        break; // Iterate again next frame
                    }
                    ImGui::SameLine();
                    
                    // Draw the item
                    if constexpr (requires(T t, Properties<UIProperties>& p) { t.Reflect(p); })
                    {
                        Properties<UIProperties> itemProps(*this);
                        values[i].Reflect(itemProps);
                    }
                    else if constexpr (std::is_same_v<T, std::string>)
                    {
                        char buf[256];
                        strncpy(buf, values[i].c_str(), sizeof(buf) - 1);
                        if (ImGui::InputText("##val", buf, sizeof(buf)))
                        {
                            values[i] = buf;
                            m_Changed = true;
                            localChanged = true;
                        }
                    }
                    else
                    {
                        ImGui::Text("Unsupported type");
                    }
                    
                    ImGui::PopID();
                    ImGui::Separator();
                }

                if (ImGui::Button(ICON_FA_PLUS " Add"))
                {
                    values.push_back({});
                    m_Changed = true;
                    localChanged = true;
                }

                ImGui::TreePop();
            }
            return localChanged;
        }

        template<typename T>
        bool Nested(const char* name, T& value)
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

        bool HasChanged() const { return m_Changed; }
        void SetChanged(bool changed) { m_Changed = changed; }
        ReflectionMode GetReflectionMode() const { return ReflectionMode::UI; }

    private:
        bool m_Changed = false;
    };
}
