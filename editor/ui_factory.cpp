#include "ui_factory.h"
#include "engine/scene/components/control_component.h"
#include "engine/scene/components/ui_action_component.h"

namespace Chained
{
std::unordered_map<std::string, UIBuilderFunc> UIFactory::s_Builders;

void UIFactory::Register(const std::string& type, UIBuilderFunc builder)
{
    s_Builders[type] = builder;
}

bool UIFactory::Create(const std::string& type, Entity entity)
{
    auto it = s_Builders.find(type);
    if (it != s_Builders.end())
    {
        it->second(entity);
        return true;
    }
    return false;
}

void UIFactory::Initialize()
{
    if (!s_Builders.empty())
    {
        return;
    }

    Register("Button", [](Entity e) { e.AddComponent<WidgetComponent>().Data = ButtonData{}; });
    Register("Panel", [](Entity e) { e.AddComponent<WidgetComponent>().Data = PanelData{}; });
    Register("Label", [](Entity e) { e.AddComponent<WidgetComponent>().Data = LabelData{}; });
    Register("Slider", [](Entity e) { e.AddComponent<WidgetComponent>().Data = SliderData{}; });
    Register("CheckBox", [](Entity e) { e.AddComponent<WidgetComponent>().Data = CheckboxData{}; });
    Register("InputText", [](Entity e) { e.AddComponent<WidgetComponent>().Data = InputTextData{}; });
    Register("ComboBox", [](Entity e) { e.AddComponent<WidgetComponent>().Data = ComboBoxData{}; });
    Register("ProgressBar", [](Entity e) { e.AddComponent<WidgetComponent>().Data = ProgressBarData{}; });
    Register("Image", [](Entity e) { e.AddComponent<WidgetComponent>().Data = ImageData{}; });
    Register("ImageButton", [](Entity e) { e.AddComponent<WidgetComponent>().Data = ImageButtonData{}; });
    Register("Separator", [](Entity e) { e.AddComponent<WidgetComponent>().Data = SeparatorData{}; });
    Register("RadioButton", [](Entity e) { e.AddComponent<WidgetComponent>().Data = RadioButtonData{}; });
    Register("ColorPicker", [](Entity e) { e.AddComponent<WidgetComponent>().Data = ColorPickerData{}; });
    Register("DragFloat", [](Entity e) { e.AddComponent<WidgetComponent>().Data = DragFloatData{}; });
    Register("DragInt", [](Entity e) { e.AddComponent<WidgetComponent>().Data = DragIntData{}; });
    Register("TreeNode", [](Entity e) { e.AddComponent<WidgetComponent>().Data = TreeNodeData{}; });
    Register("TabBar", [](Entity e) { e.AddComponent<WidgetComponent>().Data = TabBarData{}; });
    Register("TabItem", [](Entity e) { e.AddComponent<WidgetComponent>().Data = TabItemData{}; });
    Register("CollapsingHeader", [](Entity e) { e.AddComponent<WidgetComponent>().Data = CollapsingHeaderData{}; });
    Register("PlotLines", [](Entity e) { e.AddComponent<WidgetComponent>().Data = PlotLinesData{}; });
    Register("PlotHistogram", [](Entity e) { e.AddComponent<WidgetComponent>().Data = PlotHistogramData{}; });
    Register("VerticalLayoutGroup",
             [](Entity e) { e.AddComponent<WidgetComponent>().Data = VerticalLayoutGroupData{}; });
    Register("UIAction", [](Entity e) { e.AddComponent<UIActionComponent>(); });
}
} // namespace Chained
