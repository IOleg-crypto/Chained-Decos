#include "ui_factory.h"
#include "engine/scene/components/control_component.h"
#include "engine/scene/components/ui_action_component.h"

namespace CHEngine
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
        if (!s_Builders.empty()) return;

        Register("Button", [](Entity e) { e.AddComponent<ButtonControl>(); });
        Register("Panel", [](Entity e) { e.AddComponent<PanelControl>(); });
        Register("Label", [](Entity e) { e.AddComponent<LabelControl>(); });
        Register("Slider", [](Entity e) { e.AddComponent<SliderControl>(); });
        Register("CheckBox", [](Entity e) { e.AddComponent<CheckboxControl>(); });
        Register("InputText", [](Entity e) { e.AddComponent<InputTextControl>(); });
        Register("ComboBox", [](Entity e) { e.AddComponent<ComboBoxControl>(); });
        Register("ProgressBar", [](Entity e) { e.AddComponent<ProgressBarControl>(); });
        Register("Image", [](Entity e) { e.AddComponent<ImageControl>(); });
        Register("ImageButton", [](Entity e) { e.AddComponent<ImageButtonControl>(); });
        Register("Separator", [](Entity e) { e.AddComponent<SeparatorControl>(); });
        Register("RadioButton", [](Entity e) { e.AddComponent<RadioButtonControl>(); });
        Register("ColorPicker", [](Entity e) { e.AddComponent<ColorPickerControl>(); });
        Register("DragFloat", [](Entity e) { e.AddComponent<DragFloatControl>(); });
        Register("DragInt", [](Entity e) { e.AddComponent<DragIntControl>(); });
        Register("TreeNode", [](Entity e) { e.AddComponent<TreeNodeControl>(); });
        Register("TabBar", [](Entity e) { e.AddComponent<TabBarControl>(); });
        Register("TabItem", [](Entity e) { e.AddComponent<TabItemControl>(); });
        Register("CollapsingHeader", [](Entity e) { e.AddComponent<CollapsingHeaderControl>(); });
        Register("PlotLines", [](Entity e) { e.AddComponent<PlotLinesControl>(); });
        Register("PlotHistogram", [](Entity e) { e.AddComponent<PlotHistogramControl>(); });
        Register("VerticalLayoutGroup", [](Entity e) { e.AddComponent<VerticalLayoutGroup>(); });
        Register("UIAction", [](Entity e) { e.AddComponent<UIActionComponent>(); });
    }
}
