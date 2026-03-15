#include "engine/scene/entity.h"
#include "engine/scene/components.h"
#include "engine/scene/component_serializer.h"
#include "engine/core/uuid.h"
#include <unordered_map>

namespace CHEngine
{

Entity::Entity(entt::entity handle, entt::registry& registry)
    : m_EntityHandle(handle)
{
    auto* sharedPtr = registry.ctx().find<std::shared_ptr<entt::registry>>();
    if (sharedPtr)
    {
        m_Registry = *sharedPtr;
    }
    else
    {
        // Fallback or warning: this should only happen if the registry wasn't initialized by a Scene
        // We'll still set the raw pointer part of the shared_ptr if we can, but shared_ptr doesn't work that way.
        // Actually, EnTT handles usually exist within a Scene context.
    }
}

Entity::Entity(entt::entity handle, entt::registry* registry)
    : m_EntityHandle(handle)
{
    if (registry)
    {
        auto* sharedPtr = registry->ctx().find<std::shared_ptr<entt::registry>>();
        if (sharedPtr)
        {
            m_Registry = *sharedPtr;
        }
    }
}

bool Entity::IsValid() const
{
    return m_EntityHandle != entt::null && m_Registry != nullptr && m_Registry->valid(m_EntityHandle);
}

Entity Entity::Create(const std::string& name)
{
    Entity entity(m_Registry->create(), m_Registry);
    entity.AddComponent<IDComponent>();
    entity.AddComponent<TagComponent>(name.empty() ? "Entity" : name);
    entity.AddComponent<TransformComponent>();
    return entity;
}

Entity Entity::CreateWithUUID(UUID uuid, const std::string& name)
{
    Entity entity(m_Registry->create(), m_Registry);
    entity.AddComponent<IDComponent>(uuid);
    entity.AddComponent<TagComponent>(name.empty() ? "Entity" : name);
    entity.AddComponent<TransformComponent>();
    return entity;
}

Entity Entity::CreateUI(const std::string& type, const std::string& name)
{
    Entity entity = Create(name.empty() ? type : name);
    entity.AddComponent<ControlComponent>();

    if (type == "Button") entity.AddComponent<ButtonControl>();
    else if (type == "Panel") entity.AddComponent<PanelControl>();
    else if (type == "Label") entity.AddComponent<LabelControl>();
    else if (type == "Slider") entity.AddComponent<SliderControl>();
    else if (type == "CheckBox") entity.AddComponent<CheckboxControl>();
    else if (type == "InputText") entity.AddComponent<InputTextControl>();
    else if (type == "ComboBox") entity.AddComponent<ComboBoxControl>();
    else if (type == "ProgressBar") entity.AddComponent<ProgressBarControl>();
    else if (type == "Image") entity.AddComponent<ImageControl>();
    else if (type == "ImageButton") entity.AddComponent<ImageButtonControl>();
    else if (type == "Separator") entity.AddComponent<SeparatorControl>();
    else if (type == "RadioButton") entity.AddComponent<RadioButtonControl>();
    else if (type == "ColorPicker") entity.AddComponent<ColorPickerControl>();
    else if (type == "DragFloat") entity.AddComponent<DragFloatControl>();
    else if (type == "DragInt") entity.AddComponent<DragIntControl>();
    else if (type == "TreeNode") entity.AddComponent<TreeNodeControl>();
    else if (type == "TabBar") entity.AddComponent<TabBarControl>();
    else if (type == "TabItem") entity.AddComponent<TabItemControl>();
    else if (type == "CollapsingHeader") entity.AddComponent<CollapsingHeaderControl>();
    else if (type == "PlotLines") entity.AddComponent<PlotLinesControl>();
    else if (type == "PlotHistogram") entity.AddComponent<PlotHistogramControl>();

    return entity;
}

Entity Entity::Copy(entt::entity copyEntity)
{
    Entity srcEntity(copyEntity, m_Registry);
    std::string name = srcEntity.GetName();
    Entity dstEntity = Create(name);

    ComponentSerializer::Get().CopyAll(srcEntity, dstEntity);
    return dstEntity;
}

void Entity::Destroy()
{
    if (!IsValid()) return;

    std::vector<entt::entity> entitiesToDestroy;
    entitiesToDestroy.push_back(m_EntityHandle);

    size_t current = 0;
    while (current < entitiesToDestroy.size())
    {
        Entity e(entitiesToDestroy[current++], m_Registry);
        if (e.HasComponent<HierarchyComponent>())
        {
            auto& hc = e.GetComponent<HierarchyComponent>();
            for (auto child : hc.Children)
            {
                if (std::find(entitiesToDestroy.begin(), entitiesToDestroy.end(), child) == entitiesToDestroy.end())
                {
                    entitiesToDestroy.push_back(child);
                }
            }
        }
    }

    for (auto it = entitiesToDestroy.rbegin(); it != entitiesToDestroy.rend(); ++it)
    {
        if (m_Registry->valid(*it))
        {
            m_Registry->destroy(*it);
        }
    }
}

Entity Entity::FindByTag(const std::string& tag)
{
    auto view = m_Registry->view<TagComponent>();
    for (auto entity : view)
    {
        const auto& tagComp = view.get<TagComponent>(entity);
        if (tagComp.Tag == tag) return {entity, m_Registry};
    }
    return {};
}

Entity Entity::GetByUUID(UUID uuid)
{
    auto* mapStruct = m_Registry->ctx().find<EntityUUIDMap>();
    if (mapStruct && mapStruct->Map.find(uuid) != mapStruct->Map.end())
    {
        return {mapStruct->Map.at(uuid), m_Registry};
    }
    return {};
}

} // namespace CHEngine
