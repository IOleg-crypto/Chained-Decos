#include "ui_renderer.h"
#include <algorithm>

#include "engine/core/log.h"
#include "engine/runtime/application.h"
#include "engine/scene/components/component_utils.h"
#include "engine/scene/components/hierarchy_component.h"
#include "engine/scene/components/tag_component.h"
#include "engine/scene/scene.h"
#include "ui_widget_renderer.h"


namespace Chained
{

UIRenderer* UIRenderer::s_Instance = nullptr;

UIRenderer::UIRenderer()
{
}

void UIRenderer::Init(AssetManager* assetManager)
{
    s_Instance = this;
    m_AssetManager = assetManager;
    CH_CORE_INFO("[UI] Initializing UI Renderer...");
    m_Initialized = true;
}

void UIRenderer::Shutdown()
{
}


void UIRenderer::LoadProjectFonts()
{
    m_FontRegistry.LoadProjectFonts();
}

UIRect UIRenderer::GetEntityRect(Scene* scene, Entity entity, const ImVec2& viewportSize, const ImVec2& viewportPos)
{
    // For individual calls, we might still want a one-off calculation or just use the cache if available.
    // For now, let's keep it simple and use a temporary layout system update or just return empty if not in DrawCanvas.
    return m_LayoutSystem.GetEntityRect(entity);
}

void UIRenderer::ResetButtonStates(Scene* scene)
{
    if (!scene) return;
    auto& registry = scene->GetRegistry();
    auto view = registry.view<WidgetComponent>();
    for (entt::entity id : view)
    {
        view.get<WidgetComponent>(id).PressedThisFrame = false;
    }
}

std::vector<entt::entity> UIRenderer::SortUIEntities(entt::registry& registry)
{
    auto view = registry.view<ControlComponent>();
    std::vector<entt::entity> sorted(view.begin(), view.end());

    std::sort(sorted.begin(), sorted.end(), [&](entt::entity a, entt::entity b) {
        return view.get<ControlComponent>(a).ZOrder < view.get<ControlComponent>(b).ZOrder;
    });

    return sorted;
}

bool UIRenderer::RenderUIComponent(Entity entity, const ImVec2& screenPos, const ImVec2& size, bool editMode)
{
    if (!entity.HasComponent<WidgetComponent>())
        return false;

    auto& widget = entity.GetComponent<WidgetComponent>();
    return Dispatcher::Render(m_FontRegistry, m_AssetManager, entity, widget, screenPos, size);
}

void UIRenderer::DrawCanvas(Scene* scene, const ImVec2& referencePosition, const ImVec2& referenceSize, bool editMode)
{
    CH_CORE_ASSERT(scene, "Scene is null!");

    ImVec2 refSize = (referenceSize.x > 0) ? referenceSize : ImGui::GetIO().DisplaySize;
    if (refSize.x <= 0 || refSize.y <= 0) return;

    auto& registry = scene->GetRegistry();

    // 1. Reset states if frame changed
    const int frameNumber = ImGui::GetFrameCount();
    static int s_LastResetFrame = -1;
    if (s_LastResetFrame != frameNumber)
    {
        ResetButtonStates(scene);
        s_LastResetFrame = frameNumber;
    }

    // 2. Systems Update
    m_LayoutSystem.Update(scene, refSize, referencePosition);
    
    if (m_InputCooldownFrames > 0) m_InputCooldownFrames--;
    m_InputSystem.Update(registry, m_LayoutSystem, m_InputCooldownFrames);
    
    m_AnimationSystem.Update(registry, ImGui::GetIO().DeltaTime);

    // 3. Rendering
    auto uiEntities = SortUIEntities(registry);
    float scaleFactor = m_LayoutSystem.GetScaleFactor();

    ImVec2 canvasClipMin = referencePosition;
    ImVec2 canvasClipMax = {referencePosition.x + refSize.x, referencePosition.y + refSize.y};
    ImGui::PushClipRect(canvasClipMin, canvasClipMax, true);

    for (entt::entity id : uiEntities)
    {
        Entity entity(id, &registry);
        auto& control = registry.get<ControlComponent>(id);
        if (!control.IsActive) continue;

        UIRect rect = m_LayoutSystem.GetEntityRect(id);
        ImVec2 screenPos = {rect.x, rect.y};
        ImVec2 size = {rect.width, rect.height};

        bool needsClipPop = false;
        if (entity.HasComponent<HierarchyComponent>())
        {
            auto parentID = entity.GetComponent<HierarchyComponent>().Parent;
            if (parentID != entt::null)
            {
                UIRect parentRect = m_LayoutSystem.GetEntityRect(parentID);
                if (parentRect.width > 1.0f && parentRect.height > 1.0f)
                {
                    ImGui::PushClipRect({parentRect.x, parentRect.y}, 
                                        {parentRect.x + parentRect.width, parentRect.y + parentRect.height}, true);
                    needsClipPop = true;
                }
            }
        }

        if (size.x > 0.0f && size.y > 0.0f)
        {
            ImGui::SetCursorScreenPos(screenPos);
            ImGui::BeginGroup();
            ImGui::PushID((int)id);

            if (!RenderUIComponent(entity, screenPos, size, editMode))
            {
                if (entity.HasComponent<WidgetComponent>())
                {
                    const auto& widget = entity.GetComponent<WidgetComponent>();
                    if (std::holds_alternative<std::monostate>(widget.Data))
                    {
                        CH_CORE_WARN("[UI] Entity '{}' has WidgetComponent with empty WidgetData",
                                     entity.GetComponent<TagComponent>().Tag);
                    }
                }
            }

            if (editMode)
            {
                ImGui::SetCursorScreenPos(screenPos);
                ImGui::InvisibleButton("##DragZone", size);
            }

            ImGui::PopID();
            ImGui::EndGroup();
        }

        if (needsClipPop) ImGui::PopClipRect();
    }

    ImGui::PopClipRect();
}

} // namespace Chained
