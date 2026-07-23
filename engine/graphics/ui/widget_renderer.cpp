// widget_renderer.cpp
// Chained Engine — ImGui-based UI rendering for in-game widgets and HUD.
// Draws WidgetComponent hierarchy, handles Z-order sorting, scissor clipping, and font management.

#include "widget_renderer.h"
#include "engine/core/service_locator.h"
#include <algorithm>


#include "engine/scene/components/component_utils.h"
#include "engine/scene/components/hierarchy_component.h"
#include "engine/scene/components/tag_component.h"
#include "engine/scene/scene.h"
#include "ui_control_renderer.h"

namespace Chained {

WidgetRenderer::WidgetRenderer() {}

void WidgetRenderer::Initialize() {
    CH_CORE_INFO("[UI] Initializing UI Renderer...");
    m_Initialized = true;
}

void WidgetRenderer::Shutdown() {}

void WidgetRenderer::Update(Timestep ts) {}

void WidgetRenderer::LoadProjectFonts() {
    m_FontRegistry.LoadProjectFonts();
    // Eagerly preload the default project font so it's baked into the atlas texture
    // before the editor calls RefreshFontAtlasTexture.
    // Pass 'true' for allowRuntimeMutation because the editor rebuilds the texture right after this.
    m_FontRegistry.EnsureDefaultProjectFont(18.0f, true);
    
    // NOTE: atlas GPU rebuild is the caller's responsibility.
    // See project_manager.cpp / EditorLayer::ReloadEditorFonts for the single-rebuild pattern.
}

UIRect WidgetRenderer::GetEntityRect(Scene *scene, Entity entity, const ImVec2 &viewportSize, const ImVec2 &viewportPos) {
    return m_LayoutSystem.GetEntityRect((entt::entity)entity);
}

void WidgetRenderer::ResetButtonStates(Scene *scene) {
    if (!scene)
        return;

    auto &registry = scene->GetRegistry();
    auto view = registry.view<UIControlComponent>();
    for (entt::entity id : view) {
        view.get<UIControlComponent>(id).PressedThisFrame = false;
    }

    // We do NOT clear m_HasCanvasRect here anymore!
    // The previous canvas rect from Edit mode is perfectly valid for evaluating
    // hit-tests during the 1-frame suppress window. If we clear it, the hit-test
    // fails, PrevIsDown becomes false, and the next frame registers a fake human click.
}

std::vector<entt::entity> WidgetRenderer::SortUIEntities(entt::registry &registry) {
    auto view = registry.view<ControlComponent>();
    std::vector<entt::entity> sorted(view.begin(), view.end());

    // Sort by Z-order for correct back-to-front rendering sequence
    std::sort(sorted.begin(), sorted.end(), [&](entt::entity a, entt::entity b) {
        return view.get<ControlComponent>(a).ZOrder < view.get<ControlComponent>(b).ZOrder;
    });

    return sorted;
}

bool WidgetRenderer::RenderUIComponent(Entity entity, const ImVec2 &screenPos, const ImVec2 &size, bool editMode) {
    if (!entity.HasComponent<UIControlComponent>())
        return false;

    auto &control = entity.GetComponent<UIControlComponent>();
    // Delegate to the low-level ImDrawList-based control renderer
    return RenderControl(m_FontRegistry, entity, control, screenPos, size);
}

void WidgetRenderer::ProcessInput(Scene *scene, bool suppressInput) {
    if (!scene)
        return;

    // Input hit-testing needs widget layout rects. Reuse the canvas geometry
    // captured by the previous DrawCanvas; until the canvas has been drawn once
    // there is nothing on screen to click, so just reset flags via suppress.
    auto &registry = scene->GetRegistry();

    if (!m_HasCanvasRect) {
        // No canvas rect yet — reset flags only, skip hit-testing.
        UpdateUIInput(registry, m_LayoutSystem, /*suppress=*/true);
        return;
    }

    ImVec2 refSize = (m_CanvasSize.x > 0) ? m_CanvasSize : ImGui::GetIO().DisplaySize;
    m_LayoutSystem.Update(scene, refSize, m_CanvasPos);
    UpdateUIInput(registry, m_LayoutSystem, suppressInput);
}

void WidgetRenderer::DrawCanvas(Scene *scene, const ImVec2 &referencePosition, const ImVec2 &referenceSize, bool editMode) {
    CH_CORE_ASSERT(scene, "Scene is null!");

    ImVec2 refSize = (referenceSize.x > 0) ? referenceSize : ImGui::GetIO().DisplaySize;
    if (refSize.x <= 0 || refSize.y <= 0)
        return;

    auto &registry = scene->GetRegistry();

    // Cache canvas geometry so next frame's ProcessInput can hit-test without
    // depending on this render path executing.
    m_CanvasPos = referencePosition;
    m_CanvasSize = refSize;
    m_HasCanvasRect = true;

    // 1. Update layout, then render (input is handled separately in ProcessInput).
    m_LayoutSystem.Update(scene, refSize, referencePosition);
    m_AnimationSystem.Update(registry, ImGui::GetIO().DeltaTime);

    // 2. Render UI elements
    auto uiEntities = SortUIEntities(registry);

    ImVec2 canvasClipMin = referencePosition;
    ImVec2 canvasClipMax = {referencePosition.x + refSize.x, referencePosition.y + refSize.y};
    ImGui::PushClipRect(canvasClipMin, canvasClipMax, true);

    for (entt::entity id : uiEntities) {
        Entity entity(id, scene->GetRegistryPtr());

        auto &control = registry.get<ControlComponent>(id);
        if (!control.IsActive)
            continue;

        UIRect rect = m_LayoutSystem.GetEntityRect(id);
        ImVec2 screenPos = {rect.x, rect.y};
        ImVec2 size = {rect.width, rect.height};

        bool needsClipPop = false;

        // Safely retrieve parent hierarchy for scissor clipping
        if (entity.HasComponent<HierarchyComponent>()) {
            auto &hierarchy = entity.GetComponent<HierarchyComponent>();
            auto parentID = hierarchy.Parent;

            if (parentID != entt::null && registry.valid(parentID)) {
                UIRect parentRect = m_LayoutSystem.GetEntityRect(parentID);
                if (parentRect.width > 1.0f && parentRect.height > 1.0f) {
                    ImGui::PushClipRect({parentRect.x, parentRect.y},
                                        {parentRect.x + parentRect.width, parentRect.y + parentRect.height},
                                        true);
                    needsClipPop = true;
                }
            }
        }

        if (size.x > 0.0f && size.y > 0.0f) {
            ImGui::SetCursorScreenPos(screenPos);
            ImGui::BeginGroup();
            ImGui::PushID((int)id);

            // Attempt to render the control component
            if (!RenderUIComponent(entity, screenPos, size, editMode)) {
                if (entity.HasComponent<UIControlComponent>()) {
                    auto &widget = entity.GetComponent<UIControlComponent>();
                    if (std::holds_alternative<std::monostate>(widget.Data)) {
                        CH_CORE_WARN("[UI] Entity '{}' has UIControlComponent with empty ControlData",
                                     entity.GetComponent<TagComponent>().Tag);
                    }
                }
            }

            // In edit mode, overlay with an invisible button to prevent game logic from firing
            if (editMode) {
                ImGui::SetCursorScreenPos(screenPos);
                ImGui::InvisibleButton("##DragZone", size);
            }

            ImGui::PopID();
            ImGui::EndGroup();
        }

        if (needsClipPop)
            ImGui::PopClipRect();
    }

    ImGui::PopClipRect();
}
} // namespace Chained
