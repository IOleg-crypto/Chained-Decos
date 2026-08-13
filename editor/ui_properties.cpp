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
		EditorGUI::DrawPropertyLabel(name);
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

} // namespace Chained
