#ifndef CH_UI_DATA_COMPONENTS_H
#define CH_UI_DATA_COMPONENTS_H

#include "engine/assets/asset.h"

#include <variant>

namespace YAML
{
	class Emitter;
	class Node;
} // namespace YAML

namespace Chained
{

	enum WidgetType : int
	{
		WidgetType_None = 0,
		WidgetType_Button = 1,
		// 2 was removed (legacy)
		WidgetType_Label = 3,
		// 4 was removed (legacy)
		WidgetType_Slider = 5,
		WidgetType_Checkbox = 6,
		WidgetType_ProgressBar = 7,
		WidgetType_Panel = 8,
		WidgetType_Image = 9,
		WidgetType_ComboBox = 10,
		WidgetType_ImageButton = 11,
		WidgetType_InputText = 12,
		WidgetType_Separator = 13,
		WidgetType_RadioButton = 14,
		WidgetType_ColorPicker = 15,
		WidgetType_DragFloat = 16,
		WidgetType_DragInt = 17,
		WidgetType_TreeNode = 18,
		WidgetType_TabBar = 19,
		WidgetType_TabItem = 20,
		WidgetType_CollapsingHeader = 21,
		WidgetType_PlotLines = 22,
		WidgetType_PlotHistogram = 23,
		WidgetType_VerticalLayoutGroup = 24,
	};

	struct ButtonData
	{
		std::string Label = "Button";
		bool IsInteractable = true;
		bool AutoSize = false;

		void Serialize(YAML::Emitter& out) const;
		static ButtonData Deserialize(YAML::Node widgetNode);
	};

	struct PanelData
	{
		AssetHandle TextureHandle = 0;
		std::string TexturePath = "";
		bool FullScreen = false;

		void Serialize(YAML::Emitter& out) const;
		static PanelData Deserialize(YAML::Node widgetNode);
	};

	struct LabelData
	{
		std::string Text = "Text Label";
		bool AutoSize = false;

		void Serialize(YAML::Emitter& out) const;
		static LabelData Deserialize(YAML::Node widgetNode);
	};

	struct SliderData
	{
		std::string Label = "Slider";
		float Value = 0.5f;
		float Min = 0.0f;
		float Max = 1.0f;

		void Serialize(YAML::Emitter& out) const;
		static SliderData Deserialize(YAML::Node widgetNode);
	};

	struct CheckboxData
	{
		std::string Label = "Checkbox";
		bool Checked = false;

		void Serialize(YAML::Emitter& out) const;
		static CheckboxData Deserialize(YAML::Node widgetNode);
	};

	struct InputTextData
	{
		std::string Label = "Input";
		std::string Text = "";
		std::string Placeholder = "Enter text...";
		int MaxLength = 256;
		bool Multiline = false;
		bool ReadOnly = false;
		bool Password = false;
		std::vector<char> InputBuffer;

		void Serialize(YAML::Emitter& out) const;
		static InputTextData Deserialize(YAML::Node widgetNode);
	};

	struct ComboBoxData
	{
		std::string Label = "Combo";
		std::vector<std::string> Items;
		int SelectedIndex = 0;

		void Serialize(YAML::Emitter& out) const;
		static ComboBoxData Deserialize(YAML::Node widgetNode);
	};

	struct ProgressBarData
	{
		float Progress = 0.5f;
		std::string OverlayText = "";
		bool ShowPercentage = true;

		void Serialize(YAML::Emitter& out) const;
		static ProgressBarData Deserialize(YAML::Node widgetNode);
	};

	struct ImageData
	{
		AssetHandle TextureHandle = 0;
		std::string TexturePath = "";
		Color TintColor = {255, 255, 255, 255};
		Color BorderColor = {0, 0, 0, 0};

		void Serialize(YAML::Emitter& out) const;
		static ImageData Deserialize(YAML::Node widgetNode);
	};

	struct ImageButtonData
	{
		AssetHandle TextureHandle = 0;
		std::string TexturePath = "";
		std::string Label = "ImageButton";
		Color TintColor = {255, 255, 255, 255};
		Color BackgroundColor = {0, 0, 0, 0};
		int FramePadding = -1;

		void Serialize(YAML::Emitter& out) const;
		static ImageButtonData Deserialize(YAML::Node widgetNode);
	};

	struct SeparatorData
	{
		float Thickness = 1.0f;
		Color LineColor = {127, 127, 127, 255};

		void Serialize(YAML::Emitter& out) const;
		static SeparatorData Deserialize(YAML::Node widgetNode);
	};

	struct RadioButtonData
	{
		std::string Label = "RadioGroup";
		std::vector<std::string> Options = {"Option 1", "Option 2", "Option 3"};
		int SelectedIndex = 0;
		bool Horizontal = false;

		void Serialize(YAML::Emitter& out) const;
		static RadioButtonData Deserialize(YAML::Node widgetNode);
	};

	struct ColorPickerData
	{
		std::string Label = "Color";
		Color SelectedColor = {255, 255, 255, 255};
		bool ShowAlpha = true;
		bool ShowPicker = true;

		void Serialize(YAML::Emitter& out) const;
		static ColorPickerData Deserialize(YAML::Node widgetNode);
	};

	struct DragFloatData
	{
		std::string Label = "DragFloat";
		float Value = 0.0f;
		float Speed = 0.1f;
		float Min = 0.0f;
		float Max = 100.0f;
		std::string Format = "%.3f";

		void Serialize(YAML::Emitter& out) const;
		static DragFloatData Deserialize(YAML::Node widgetNode);
	};

	struct DragIntData
	{
		std::string Label = "DragInt";
		int Value = 0;
		float Speed = 1.0f;
		int Min = 0;
		int Max = 100;
		std::string Format = "%d";

		void Serialize(YAML::Emitter& out) const;
		static DragIntData Deserialize(YAML::Node widgetNode);
	};

	struct TreeNodeData
	{
		std::string Label = "TreeNode";
		bool IsOpen = false;
		bool DefaultOpen = false;
		bool IsLeaf = false;

		void Serialize(YAML::Emitter& out) const;
		static TreeNodeData Deserialize(YAML::Node widgetNode);
	};

	struct TabBarData
	{
		std::string Label = "TabBar";
		bool Reorderable = true;
		bool AutoSelectNewTabs = true;

		void Serialize(YAML::Emitter& out) const;
		static TabBarData Deserialize(YAML::Node widgetNode);
	};

	struct TabItemData
	{
		std::string Label = "Tab";
		bool IsOpen = true;
		bool Selected = false;

		void Serialize(YAML::Emitter& out) const;
		static TabItemData Deserialize(YAML::Node widgetNode);
	};

	struct CollapsingHeaderData
	{
		std::string Label = "Header";
		bool IsOpen = false;
		bool DefaultOpen = false;

		void Serialize(YAML::Emitter& out) const;
		static CollapsingHeaderData Deserialize(YAML::Node widgetNode);
	};

	enum class PlotMode : uint8_t
	{
		Lines,
		Histogram,
	};

	struct PlotData
	{
		PlotMode Mode = PlotMode::Lines;
		std::string Label = "Plot";
		std::vector<float> Values = {0.0f, 0.5f, 1.0f, 0.5f, 0.0f};
		std::string OverlayText = "";
		float ScaleMin = 0.0f;
		float ScaleMax = 1.0f;
		glm::vec2 GraphSize = {0, 80};

		void Serialize(YAML::Emitter& out) const;
		static PlotData Deserialize(YAML::Node widgetNode);
	};

	// Backward-compatible aliases for existing code
	using PlotLinesData = PlotData;
	using PlotHistogramData = PlotData;

	struct VerticalLayoutGroupData
	{
		float Spacing = 10.0f;
		glm::vec2 Padding = {10, 10};

		void Serialize(YAML::Emitter& out) const;
		static VerticalLayoutGroupData Deserialize(YAML::Node widgetNode);
	};

	using ControlData =
		std::variant<std::monostate, ButtonData, PanelData, LabelData, SliderData, CheckboxData, InputTextData,
					 ComboBoxData, ProgressBarData, ImageData, ImageButtonData, SeparatorData, RadioButtonData,
					 ColorPickerData, DragFloatData, DragIntData, TreeNodeData, TabBarData, TabItemData,
					 CollapsingHeaderData, PlotData, VerticalLayoutGroupData>;

	ControlData DeserializeControlData(int widgetType, YAML::Node widgetNode);

	ControlData CreateDefaultWidgetData(WidgetType type);

	const char* WidgetTypeName(WidgetType type);

} // namespace Chained

#endif
