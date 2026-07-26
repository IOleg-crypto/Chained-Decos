#ifndef CH_UI_PROPERTIES_H
#define CH_UI_PROPERTIES_H

#include "thirdparty/IconsFontAwesome6.h"
#include "gui.h"
#include "engine/reflection/reflection.h"
#include "imgui.h"
#include "imgui_internal.h"


#include <algorithm>
#include <cstring>
#include <functional>
#include <string_view>
#include <vector>


namespace Chained
{
// Implementation of the Archive concept for ImGui UI
class UIProperties : public IPropertyArchive
{
public:
    UIProperties() = default;

    virtual ReflectionMode GetReflectionMode() const override
    {
        return ReflectionMode::UI;  
    }
    virtual bool HasChanged() const override
    {
        return m_Changed;
    }
    virtual void SetChanged(bool changed) override
    {
        m_Changed = changed;
    }

    // IPropertyArchive overrides
    virtual bool Property(const char* name, int& value, const PropertyMeta& meta = {}) override
    {
        return PropertyInternal(name, value, meta);
    }
    virtual bool Property(const char* name, float& value, const PropertyMeta& meta = {}) override
    {
        return PropertyInternal(name, value, meta);
    }
    virtual bool Property(const char* name, bool& value, const PropertyMeta& meta = {}) override
    {
        return PropertyInternal(name, value, meta);
    }
    virtual bool Property(const char* name, std::string& value, const PropertyMeta& meta = {}) override
    {
        return PropertyInternal(name, value, meta);
    }
    virtual bool Property(const char* name, glm::vec2& value, const PropertyMeta& meta = {}) override
    {
        return PropertyInternal(name, value, meta);
    }
    virtual bool Property(const char* name, glm::vec3& value, const PropertyMeta& meta = {}) override
    {
        return PropertyInternal(name, value, meta);
    }
    virtual bool Property(const char* name, glm::vec4& value, const PropertyMeta& meta = {}) override
    {
        return PropertyInternal(name, value, meta);
    }
    virtual bool Property(const char* name, Color& value, const PropertyMeta& meta = {}) override
    {
        return PropertyInternal(name, value, meta);
    }
    virtual bool Enum(const char* name, int& value, const char** names, int count,
                      const PropertyMeta& meta = {}) override
    {
        return EnumPropertyInternal(name, value, names, count, meta);
    }

    virtual bool StringEnum(const char* name, std::string& value, const std::vector<std::string>& options,
                      const PropertyMeta& meta = {}) override
    {
        return StringEnumInternal(name, value, options, meta);
    }

    virtual bool Property(const char* name, uint64_t& value, const PropertyMeta& meta = {}) override
    {
        return Handle(name, value, meta);
    }
    virtual bool Handle(const char* name, uint64_t& value, const PropertyMeta& meta = {}) override
    {
        return HandleInternal(name, value);
    }
    virtual bool File(const char* name, std::string& value, const char* extensions = nullptr,
                      const PropertyMeta& meta = {}) override
    {
        return FileInternal(name, value, extensions, meta);
    }
    virtual bool Action(const char* label, std::function<void()> func) override
    {
        return ActionInternal(label, func);
    }
    virtual void Header(const char* label) override
    {
        HeaderInternal(label);
    }
    virtual void Separator() override
    {
        SeparatorInternal();
    }
    virtual bool BeginGroup(const char* label, bool defaultOpen = true) override
    {
        return BeginGroupInternal(label, defaultOpen);
    }
    virtual void EndGroup() override
    {
        EndGroupInternal();
    }

    bool HasFinished() const
    {
        return m_Started && m_Finished;
    }
    bool HasStarted() const
    {
        return m_Started;
    }

    // Template methods for non-virtual calls (still used by Properties<UIProperties>)
    template <typename T> bool Property(const char* name, T& value)
    {
        return PropertyInternal(name, value, {});
    }
    template <typename T_Enum> bool Property(const char* name, T_Enum& value, const char** names, int count)
    {
        return EnumPropertyInternal(name, (int&)value, names, count, {});
    }
    template <typename T> bool Property(const char* name, T& value, const PropertyMeta& meta)
    {
        return PropertyInternal(name, value, meta);
    }
    template <typename T_Enum>
    bool Property(const char* name, T_Enum& value, const char** names, int count, const PropertyMeta& meta)
    {
        return EnumPropertyInternal(name, (int&)value, names, count, meta);
    }

    template <typename T> bool Sequence(const char* name, std::vector<T>& values, bool allowAddRemove = true)
    {
        if (ImGui::GetCurrentTable() != nullptr)
        {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::AlignTextToFramePadding();
        }

        bool localChanged = false;
        ImGuiTreeNodeFlags flags =
            ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth;
        if (ImGui::GetCurrentTable() != nullptr)
        {
            flags |= ImGuiTreeNodeFlags_SpanAllColumns;
        }

        if (ImGui::TreeNodeEx(name, flags))
        {
            for (size_t i = 0; i < values.size(); i++)
            {
                ImGui::PushID((int)i);
                if (ImGui::GetCurrentTable() != nullptr)
                {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(1);
                }

                if (allowAddRemove && ImGui::Button(ICON_FA_TRASH))
                {
                    values.erase(values.begin() + i);
                    m_Changed = localChanged = true;
                    ImGui::PopID();
                    break;
                }

                if (allowAddRemove)
                {
                    ImGui::SameLine();
                }

                if constexpr (std::is_arithmetic_v<T> || std::is_same_v<T, std::string> || is_variant_v<T> ||
                              std::is_enum_v<T> || (std::is_integral_v<T> && std::is_unsigned_v<T>))
                {
                    Properties<UIProperties> itemProps(*this);
                    if (itemProps.Property("##val", values[i]))
                    {
                        m_Changed = localChanged = true;
                    }
                }
                else if constexpr (is_rfl_component<T>::value)
                {
                    char label[32];
                    sprintf(label, "Item %d", (int)i);
                    if (ImGui::TreeNodeEx(label, ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth))
                    {
                        EditorGUI::BeginPropertyGrid();
                        Properties<UIProperties> itemProps(*this);
                        ReflectFromRfl(values[i], itemProps);
                        EditorGUI::EndPropertyGrid();
                        ImGui::TreePop();
                    }
                }
                else if constexpr (requires(T t, Properties<UIProperties>& p) { t.Reflect(p); })
                {
                    char label[32];
                    sprintf(label, "Item %d", (int)i);
                    if (ImGui::TreeNodeEx(label, ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth))
                    {
                        EditorGUI::BeginPropertyGrid();
                        Properties<UIProperties> itemProps(*this);
                        values[i].Reflect(itemProps);
                        EditorGUI::EndPropertyGrid();
                        ImGui::TreePop();
                    }
                }
                else if constexpr (std::is_same_v<T, std::string>)
                {
                    char buf[256];
                    strncpy(buf, values[i].c_str(), sizeof(buf) - 1);
                    buf[sizeof(buf) - 1] = '\0';
                    ImGui::SetNextItemWidth(-1);
                    if (ImGui::InputText("##val", buf, sizeof(buf)))
                    {
                        values[i] = buf;
                        m_Changed = localChanged = true;
                    }
                }
                else
                {
                    ImGui::Text("Item %d", (int)i);
                }

                ImGui::PopID();
                if constexpr (!requires(T t, Properties<UIProperties>& p) { t.Reflect(p); })
                {
                    ImGui::Separator();
                }
            }

            ImGui::Spacing();
            if (ImGui::GetCurrentTable() != nullptr)
            {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(1);
            }

            if (allowAddRemove && EditorGUI::ActionButton(ICON_FA_PLUS, "Add New Item"))
            {
                values.emplace_back();
                m_Changed = localChanged = true;
            }
            ImGui::TreePop();
        }
        return localChanged;
    }

    virtual void BeginSequence(const char* name, size_t& size) override
    {
        if (ImGui::GetCurrentTable() != nullptr)
        {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::AlignTextToFramePadding();
        }

        ImGuiTreeNodeFlags flags =
            ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth;
        if (ImGui::GetCurrentTable() != nullptr)
        {
            flags |= ImGuiTreeNodeFlags_SpanAllColumns;
        }

        m_InSequence = ImGui::TreeNodeEx(name, flags);
        if (m_InSequence && ImGui::GetCurrentTable() != nullptr)
        {
            ImGui::TableSetColumnIndex(1);
        }
    }

    virtual void EndSequence() override
    {
        if (m_InSequence)
        {
            ImGui::TreePop();
            m_InSequence = false;
        }
    }

    virtual bool Nested(const char* name, std::function<void(IPropertyArchiveBase&)> callback) override
    {
        if (ImGui::TreeNodeEx(name, ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen))
        {
            callback(*this);
            ImGui::TreePop();
            return true; // Simplified: assume changed if we opened and called callback
        }
        return false;
    }

private:
    bool m_InSequence = false;

public:
    // --- Legacy types or internal helpers if needed ---
    // Note: Most templates are now handled by the base class Properties<T_Archive>.

private:
    template <typename T> bool PropertyInternal(const char* name, T& value, const PropertyMeta& meta)
    {
        bool changed = false;
        ImGui::BeginDisabled(meta.ReadOnly);
        if constexpr (std::is_same_v<T, float>)
        {
            changed = EditorGUI::Property(name, value, meta.Speed, meta.MinValue, meta.MaxValue);
        }
        else if constexpr (std::is_same_v<T, int>)
        {
            changed = EditorGUI::Property(name, value, (int)meta.MinValue, (int)meta.MaxValue);
        }
        else if constexpr (std::is_same_v<T, std::string>)
        {
            changed = StringProperty(name, value, meta);
        }
        else if constexpr (is_variant_v<T>)
        {
            changed = std::visit([&](auto&& v) { return EditorGUI::Property(name, v); }, value);
        }
        else if constexpr (std::is_same_v<T, float[4]>)
        {
            changed = EditorGUI::Property(name, *(glm::vec4*)&value);
        }
        else
        {
            if constexpr (requires { EditorGUI::Property(name, value); })
            {
                changed = EditorGUI::Property(name, value);
            }
        }
        ImGui::EndDisabled();

        if (!meta.Tooltip.empty() && ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("%s", meta.Tooltip.c_str());
        }
        UpdateState(changed);
        return changed;
    }

    bool EnumPropertyInternal(const char* name, int& value, const char** names, int count, const PropertyMeta& meta);
    bool StringEnumInternal(const char* name, std::string& value, const std::vector<std::string>& options, const PropertyMeta& meta);
    bool HandleInternal(const char* name, uint64_t& value);
    bool FileInternal(const char* name, std::string& path, const char* extensions, const PropertyMeta& meta);
    bool ActionInternal(const char* label, std::function<void()> func);
    void HeaderInternal(const char* label);
    void SeparatorInternal();
    bool BeginGroupInternal(const char* label, bool defaultOpen = true);
    void EndGroupInternal();
    void UpdateState(bool changed);
    bool StringProperty(const char* name, std::string& value, const PropertyMeta& meta);

    bool m_Changed = false;
    bool m_Started = false;
    bool m_Finished = false;
};
} // namespace Chained

#endif // CH_UI_PROPERTIES_H
