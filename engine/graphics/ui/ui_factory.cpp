#include "ui_factory.h"
#include "engine/scene/components/control_component.h"
#include "engine/scene/components/ui_action_component.h"
#include "engine/scene/components.h" // For ControlData types
#include "engine/core/log.h"

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
    CH_CORE_WARN("[UIFactory] Unknown UI type '{}' for entity — UIControlComponent::Data will be empty", type);
    return false;
}

void UIFactory::Clear()
{
    s_Builders.clear();
}

void UIFactory::Initialize()
{
    if (!s_Builders.empty())
    {
        return;
    }

    Register("Button", [](Entity e) { e.AddComponent<UIControlComponent>().Data = ButtonData{}; });
    Register("Panel", [](Entity e) { e.AddComponent<UIControlComponent>().Data = PanelData{}; });
    Register("Label", [](Entity e) { e.AddComponent<UIControlComponent>().Data = LabelData{}; });
    Register("Slider", [](Entity e) { e.AddComponent<UIControlComponent>().Data = SliderData{}; });
    Register("Checkbox", [](Entity e) { e.AddComponent<UIControlComponent>().Data = CheckboxData{}; });
    Register("InputText", [](Entity e) { e.AddComponent<UIControlComponent>().Data = InputTextData{}; });
    Register("ComboBox", [](Entity e) { e.AddComponent<UIControlComponent>().Data = ComboBoxData{}; });
    Register("ProgressBar", [](Entity e) { e.AddComponent<UIControlComponent>().Data = ProgressBarData{}; });
    Register("Image", [](Entity e) { e.AddComponent<UIControlComponent>().Data = ImageData{}; });
    Register("ImageButton", [](Entity e) { e.AddComponent<UIControlComponent>().Data = ImageButtonData{}; });
    Register("Separator", [](Entity e) { e.AddComponent<UIControlComponent>().Data = SeparatorData{}; });
    Register("RadioButton", [](Entity e) { e.AddComponent<UIControlComponent>().Data = RadioButtonData{}; });
    Register("ColorPicker", [](Entity e) { e.AddComponent<UIControlComponent>().Data = ColorPickerData{}; });
    Register("DragFloat", [](Entity e) { e.AddComponent<UIControlComponent>().Data = DragFloatData{}; });
    Register("DragInt", [](Entity e) { e.AddComponent<UIControlComponent>().Data = DragIntData{}; });
    Register("TreeNode", [](Entity e) { e.AddComponent<UIControlComponent>().Data = TreeNodeData{}; });
    Register("TabBar", [](Entity e) { e.AddComponent<UIControlComponent>().Data = TabBarData{}; });
    Register("TabItem", [](Entity e) { e.AddComponent<UIControlComponent>().Data = TabItemData{}; });
    Register("CollapsingHeader", [](Entity e) { e.AddComponent<UIControlComponent>().Data = CollapsingHeaderData{}; });
    Register("PlotLines", [](Entity e) {
        PlotData d; d.Mode = PlotMode::Lines;
        e.AddComponent<UIControlComponent>().Data = d;
    });
    Register("PlotHistogram", [](Entity e) {
        PlotData d; d.Mode = PlotMode::Histogram;
        e.AddComponent<UIControlComponent>().Data = d;
    });
    Register("VerticalLayoutGroup",
             [](Entity e) { e.AddComponent<UIControlComponent>().Data = VerticalLayoutGroupData{}; });
    Register("UIAction", [](Entity e) { e.AddComponent<UIActionComponent>(); });
}
} // namespace Chained
