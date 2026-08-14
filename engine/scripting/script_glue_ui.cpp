#include "script_glue_ui.h"

namespace Chained
{
	uint8_t ButtonControl_IsClicked(uint64_t entityID)
	{
		Entity entity = GetEntity(entityID);
		if (entity && entity.HasComponent<UIControlComponent>())
		{
			auto& widget = entity.GetComponent<UIControlComponent>();
			if (widget.PressedThisFrame)
			{
				widget.PressedThisFrame = false; // consume on read
				return true;
			}
		}
		return false;
	}

	uint8_t ButtonControl_IsDown(uint64_t entityID)
	{
		Entity entity = GetEntity(entityID);
		return entity && entity.HasComponent<UIControlComponent>() ? entity.GetComponent<UIControlComponent>().IsDown
																   : false;
	}

	const Coral::UCChar* ButtonControl_GetLabel(uint64_t entityID)
	{
		Entity entity = GetEntity(entityID);
		if (entity && entity.HasComponent<UIControlComponent>())
		{
			auto& widget = entity.GetComponent<UIControlComponent>();
			if (std::holds_alternative<ButtonData>(widget.Data))
			{
				return GlueStringPool::ReturnString(std::get<ButtonData>(widget.Data).Label);
			}
		}
		return nullptr;
	}
	void ButtonControl_SetLabel(uint64_t entityID, const Coral::UCChar* label)
	{
		Entity entity = GetEntity(entityID);
		if (entity && entity.HasComponent<UIControlComponent>() && label)
		{
			auto& widget = entity.GetComponent<UIControlComponent>();
			if (std::holds_alternative<ButtonData>(widget.Data))
			{
				std::get<ButtonData>(widget.Data).Label = ch_u16_to_string(label);
			}
		}
	}

	void CheckboxControl_SetChecked(uint64_t entityID, uint8_t checked)
	{
		Entity entity = GetEntity(entityID);
		if (entity && entity.HasComponent<UIControlComponent>())
		{
			auto& widget = entity.GetComponent<UIControlComponent>();
			if (std::holds_alternative<CheckboxData>(widget.Data))
			{
				std::get<CheckboxData>(widget.Data).Checked = checked;
			}
		}
	}

	const Coral::UCChar* LabelControl_GetText(uint64_t entityID)
	{
		Entity entity = GetEntity(entityID);
		if (entity && entity.HasComponent<UIControlComponent>())
		{
			auto& widget = entity.GetComponent<UIControlComponent>();
			if (std::holds_alternative<LabelData>(widget.Data))
			{
				return GlueStringPool::ReturnString(std::get<LabelData>(widget.Data).Text);
			}
		}
		return nullptr;
	}
	void LabelControl_SetText(uint64_t entityID, const Coral::UCChar* text)
	{
		Entity entity = GetEntity(entityID);
		if (entity && entity.HasComponent<UIControlComponent>() && text)
		{
			auto& widget = entity.GetComponent<UIControlComponent>();
			if (std::holds_alternative<LabelData>(widget.Data))
			{
				std::get<LabelData>(widget.Data).Text = ch_u16_to_string(text);
			}
		}
	}

	const Coral::UCChar* SliderControl_GetLabel(uint64_t entityID)
	{
		Entity entity = GetEntity(entityID);
		if (entity && entity.HasComponent<UIControlComponent>())
		{
			auto& widget = entity.GetComponent<UIControlComponent>();
			if (std::holds_alternative<SliderData>(widget.Data))
			{
				return GlueStringPool::ReturnString(std::get<SliderData>(widget.Data).Label);
			}
		}
		return nullptr;
	}
	void SliderControl_SetLabel(uint64_t entityID, const Coral::UCChar* label)
	{
		Entity entity = GetEntity(entityID);
		if (entity && entity.HasComponent<UIControlComponent>() && label)
		{
			auto& widget = entity.GetComponent<UIControlComponent>();
			if (std::holds_alternative<SliderData>(widget.Data))
			{
				std::get<SliderData>(widget.Data).Label = ch_u16_to_string(label);
			}
		}
	}
	float SliderControl_GetValue(uint64_t entityID)
	{
		Entity entity = GetEntity(entityID);
		if (entity && entity.HasComponent<UIControlComponent>())
		{
			auto& widget = entity.GetComponent<UIControlComponent>();
			if (std::holds_alternative<SliderData>(widget.Data))
			{
				return std::get<SliderData>(widget.Data).Value;
			}
		}
		return 0.0f;
	}
	void SliderControl_SetValue(uint64_t entityID, float value)
	{
		Entity entity = GetEntity(entityID);
		if (entity && entity.HasComponent<UIControlComponent>())
		{
			auto& widget = entity.GetComponent<UIControlComponent>();
			if (std::holds_alternative<SliderData>(widget.Data))
			{
				std::get<SliderData>(widget.Data).Value = value;
			}
		}
	}
	float SliderControl_GetMin(uint64_t entityID)
	{
		Entity entity = GetEntity(entityID);
		if (entity && entity.HasComponent<UIControlComponent>())
		{
			auto& widget = entity.GetComponent<UIControlComponent>();
			if (std::holds_alternative<SliderData>(widget.Data))
			{
				return std::get<SliderData>(widget.Data).Min;
			}
		}
		return 0.0f;
	}
	void SliderControl_SetMin(uint64_t entityID, float minVal)
	{
		Entity entity = GetEntity(entityID);
		if (entity && entity.HasComponent<UIControlComponent>())
		{
			auto& widget = entity.GetComponent<UIControlComponent>();
			if (std::holds_alternative<SliderData>(widget.Data))
			{
				std::get<SliderData>(widget.Data).Min = minVal;
			}
		}
	}
	float SliderControl_GetMax(uint64_t entityID)
	{
		Entity entity = GetEntity(entityID);
		if (entity && entity.HasComponent<UIControlComponent>())
		{
			auto& widget = entity.GetComponent<UIControlComponent>();
			if (std::holds_alternative<SliderData>(widget.Data))
			{
				return std::get<SliderData>(widget.Data).Max;
			}
		}
		return 0.0f;
	}
	void SliderControl_SetMax(uint64_t entityID, float maxVal)
	{
		Entity entity = GetEntity(entityID);
		if (entity && entity.HasComponent<UIControlComponent>())
		{
			auto& widget = entity.GetComponent<UIControlComponent>();
			if (std::holds_alternative<SliderData>(widget.Data))
			{
				std::get<SliderData>(widget.Data).Max = maxVal;
			}
		}
	}

	float ProgressBarControl_GetProgress(uint64_t entityID)
	{
		Entity entity = GetEntity(entityID);
		if (entity && entity.HasComponent<UIControlComponent>())
		{
			auto& widget = entity.GetComponent<UIControlComponent>();
			if (std::holds_alternative<ProgressBarData>(widget.Data))
			{
				return std::get<ProgressBarData>(widget.Data).Progress;
			}
		}
		return 0.0f;
	}
	void ProgressBarControl_SetProgress(uint64_t entityID, float progress)
	{
		Entity entity = GetEntity(entityID);
		if (entity && entity.HasComponent<UIControlComponent>())
		{
			auto& widget = entity.GetComponent<UIControlComponent>();
			if (std::holds_alternative<ProgressBarData>(widget.Data))
			{
				std::get<ProgressBarData>(widget.Data).Progress = progress;
			}
		}
	}
	const Coral::UCChar* ProgressBarControl_GetOverlayText(uint64_t entityID)
	{
		Entity entity = GetEntity(entityID);
		if (entity && entity.HasComponent<UIControlComponent>())
		{
			auto& widget = entity.GetComponent<UIControlComponent>();
			if (std::holds_alternative<ProgressBarData>(widget.Data))
			{
				return GlueStringPool::ReturnString(std::get<ProgressBarData>(widget.Data).OverlayText);
			}
		}
		return nullptr;
	}
	void ProgressBarControl_SetOverlayText(uint64_t entityID, const Coral::UCChar* text)
	{
		Entity entity = GetEntity(entityID);
		if (entity && entity.HasComponent<UIControlComponent>() && text)
		{
			auto& widget = entity.GetComponent<UIControlComponent>();
			if (std::holds_alternative<ProgressBarData>(widget.Data))
			{
				std::get<ProgressBarData>(widget.Data).OverlayText = ch_u16_to_string(text);
			}
		}
	}
	uint8_t ProgressBarControl_GetShowPercentage(uint64_t entityID)
	{
		Entity entity = GetEntity(entityID);
		if (entity && entity.HasComponent<UIControlComponent>())
		{
			auto& widget = entity.GetComponent<UIControlComponent>();
			if (std::holds_alternative<ProgressBarData>(widget.Data))
			{
				return std::get<ProgressBarData>(widget.Data).ShowPercentage;
			}
		}
		return false;
	}
	void ProgressBarControl_SetShowPercentage(uint64_t entityID, uint8_t show)
	{
		Entity entity = GetEntity(entityID);
		if (entity && entity.HasComponent<UIControlComponent>())
		{
			auto& widget = entity.GetComponent<UIControlComponent>();
			if (std::holds_alternative<ProgressBarData>(widget.Data))
			{
				std::get<ProgressBarData>(widget.Data).ShowPercentage = show;
			}
		}
	}

	uint8_t WidgetControl_GetActive(uint64_t entityID)
	{
		Entity entity = GetEntity(entityID);
		return entity && entity.HasComponent<ControlComponent>() ? entity.GetComponent<ControlComponent>().IsActive
																 : false;
	}
	void WidgetControl_SetActive(uint64_t entityID, uint8_t active)
	{
		Entity entity = GetEntity(entityID);
		if (entity && entity.HasComponent<ControlComponent>())
		{
			entity.GetComponent<ControlComponent>().IsActive = active;
		}
	}

	const Coral::UCChar* WidgetControl_GetTextColor(uint64_t entityID)
	{
		// Returns "R G B A" as space-separated integers
		Entity entity = GetEntity(entityID);
		if (entity && entity.HasComponent<UIControlComponent>())
		{
			auto& c = entity.GetComponent<UIControlComponent>().TextStyle.TextColor;
			std::string s =
				std::to_string(c.r) + " " + std::to_string(c.g) + " " + std::to_string(c.b) + " " + std::to_string(c.a);
			return GlueStringPool::ReturnString(s);
		}
		return nullptr;
	}
	void WidgetControl_SetTextColorRGBA(uint64_t entityID, int r, int g, int b, int a)
	{
		Entity entity = GetEntity(entityID);
		if (entity && entity.HasComponent<UIControlComponent>())
		{
			auto& col = entity.GetComponent<UIControlComponent>().TextStyle.TextColor;
			col.r = (uint8_t)std::clamp(r, 0, 255);
			col.g = (uint8_t)std::clamp(g, 0, 255);
			col.b = (uint8_t)std::clamp(b, 0, 255);
			col.a = (uint8_t)std::clamp(a, 0, 255);
		}
	}

	uint8_t CheckboxControl_GetChecked(uint64_t entityID)
	{
		Entity entity = GetEntity(entityID);
		if (entity && entity.HasComponent<UIControlComponent>())
		{
			auto& widget = entity.GetComponent<UIControlComponent>();
			if (std::holds_alternative<CheckboxData>(widget.Data))
			{
				return std::get<CheckboxData>(widget.Data).Checked;
			}
		}
		return false;
	}
	int ComboBoxControl_GetSelectedIndex(uint64_t entityID)
	{
		Entity entity = GetEntity(entityID);
		if (entity && entity.HasComponent<UIControlComponent>())
		{
			auto& widget = entity.GetComponent<UIControlComponent>();
			if (std::holds_alternative<ComboBoxData>(widget.Data))
			{
				return std::get<ComboBoxData>(widget.Data).SelectedIndex;
			}
		}
		return 0;
	}
	void ComboBoxControl_SetSelectedIndex(uint64_t entityID, int index)
	{
		Entity entity = GetEntity(entityID);
		if (entity && entity.HasComponent<UIControlComponent>())
		{
			auto& widget = entity.GetComponent<UIControlComponent>();
			if (std::holds_alternative<ComboBoxData>(widget.Data))
			{
				std::get<ComboBoxData>(widget.Data).SelectedIndex = index;
			}
		}
	}
	void ComboBoxControl_AddItem(uint64_t entityID, const Coral::UCChar* item)
	{
		Entity entity = GetEntity(entityID);
		if (entity && entity.HasComponent<UIControlComponent>() && item)
		{
			auto& widget = entity.GetComponent<UIControlComponent>();
			if (std::holds_alternative<ComboBoxData>(widget.Data))
			{
				std::get<ComboBoxData>(widget.Data).Items.push_back(ch_u16_to_string(item));
			}
		}
	}
	void ComboBoxControl_ClearItems(uint64_t entityID)
	{
		Entity entity = GetEntity(entityID);
		if (entity && entity.HasComponent<UIControlComponent>())
		{
			auto& widget = entity.GetComponent<UIControlComponent>();
			if (std::holds_alternative<ComboBoxData>(widget.Data))
			{
				auto& combo = std::get<ComboBoxData>(widget.Data);
				combo.Items.clear();
				combo.SelectedIndex = 0;
			}
		}
	}
	int ComboBoxControl_GetItemCount(uint64_t entityID)
	{
		Entity entity = GetEntity(entityID);
		if (entity && entity.HasComponent<UIControlComponent>())
		{
			auto& widget = entity.GetComponent<UIControlComponent>();
			if (std::holds_alternative<ComboBoxData>(widget.Data))
			{
				return (int)std::get<ComboBoxData>(widget.Data).Items.size();
			}
		}
		return 0;
	}
	const Coral::UCChar* ComboBoxControl_GetItem(uint64_t entityID, int index)
	{
		Entity entity = GetEntity(entityID);
		if (entity && entity.HasComponent<UIControlComponent>())
		{
			auto& widget = entity.GetComponent<UIControlComponent>();
			if (std::holds_alternative<ComboBoxData>(widget.Data))
			{
				auto& combo = std::get<ComboBoxData>(widget.Data);
				if (index >= 0 && index < (int)combo.Items.size())
				{
					return GlueStringPool::ReturnString(combo.Items[index]);
				}
			}
		}
		return nullptr;
	}
	const Coral::UCChar* InputTextControl_GetText(uint64_t entityID)
	{
		Entity entity = GetEntity(entityID);
		if (entity && entity.HasComponent<UIControlComponent>())
		{
			auto& widget = entity.GetComponent<UIControlComponent>();
			if (std::holds_alternative<InputTextData>(widget.Data))
			{
				auto& input = std::get<InputTextData>(widget.Data);
				// Sync text from InputBuffer if it has content
				if (!input.InputBuffer.empty())
				{
					input.Text = input.InputBuffer.data();
				}
				return GlueStringPool::ReturnString(input.Text);
			}
		}
		return nullptr;
	}

	void InputTextControl_SetText(uint64_t entityID, const Coral::UCChar* text)
	{
		Entity entity = GetEntity(entityID);
		if (entity && entity.HasComponent<UIControlComponent>() && text)
		{
			auto& widget = entity.GetComponent<UIControlComponent>();
			if (std::holds_alternative<InputTextData>(widget.Data))
			{
				auto& input = std::get<InputTextData>(widget.Data);
				input.Text = ch_u16_to_string(text);
				// Reset buffer so it picks up the new text on next render
				input.InputBuffer.clear();
			}
		}
	}

	uint8_t InputTextControl_HasChanged(uint64_t entityID)
	{
		Entity entity = GetEntity(entityID);
		if (entity && entity.HasComponent<UIControlComponent>())
		{
			return entity.GetComponent<UIControlComponent>().ValueChanged;
		}
		return false;
	}

	void InputTextControl_ClearChanged(uint64_t entityID)
	{
		Entity entity = GetEntity(entityID);
		if (entity && entity.HasComponent<UIControlComponent>())
		{
			entity.GetComponent<UIControlComponent>().ValueChanged = false;
		}
	}

	void UI_Text(const Coral::UCChar* text)
	{
		if (ImGui::GetCurrentContext() == nullptr || !ImGui::GetCurrentContext()->WithinFrameScope || !text)
		{
			return;
		}

		std::string strText = ch_u16_to_string(text);

		auto window = ImGui::GetCurrentContext()->CurrentWindow;
		if (window && !window->SkipItems)
		{
			ImGui::Text("%s", strText.c_str());
		}
		else
		{
			ImGui::GetForegroundDrawList()->AddText({10, 10}, IM_COL32(255, 255, 0, 255), strText.c_str());
		}
	}

	uint8_t UI_Button(const Coral::UCChar* label)
	{
		if (ImGui::GetCurrentContext() == nullptr || !ImGui::GetCurrentContext()->WithinFrameScope || !label)
		{
			return false;
		}

		std::string strLabel = ch_u16_to_string(label);
		return ImGui::Button(strLabel.c_str());
	}

} // namespace Chained
