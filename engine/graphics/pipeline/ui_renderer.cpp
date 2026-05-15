#include "ui_renderer.h"
#include "engine/assets/asset_manager.h"
#include "engine/core/log.h"
#include "engine/core/service_locator.h"
#include "engine/graphics/loaders/font_loader.h"
#include "engine/scene/components/component_utils.h"
#include "engine/scene/components/hierarchy_component.h"
#include "engine/scene/components/tag_component.h"
#include "engine/scene/scene.h"
#include "ui_widget_renderer.h"

namespace CHEngine
{

void UIRenderer::OnInit()
{
    CH_CORE_INFO("[UI] Initializing UI Renderer...");
    ServiceLocator::Get<AssetManager>().RegisterLoader(AssetType::Font, std::make_unique<FontLoader>());
    m_Initialized = true;
}

void UIRenderer::OnShutdown()
{
}

void UIRenderer::LoadProjectFonts()
{
    m_FontRegistry.LoadProjectFonts();
}

UIRect UIRenderer::GetEntityRect(Scene* scene, Entity entity, const ImVec2& viewportSize, const ImVec2& viewportPos)
{
    if (!scene) return {0, 0, 0, 0};

    const CanvasSettings& canvas = scene->GetSettings().Canvas;
    float scaleFactor = 1.0f;

    if (canvas.ScaleMode == CanvasScaleMode::ScaleWithScreenSize && canvas.ReferenceResolution.x > 0 &&
        canvas.ReferenceResolution.y > 0)
    {
        float scaleX = viewportSize.x / canvas.ReferenceResolution.x;
        float scaleY = viewportSize.y / canvas.ReferenceResolution.y;
        scaleFactor = scaleX * (1.0f - canvas.MatchWidthOrHeight) + scaleY * canvas.MatchWidthOrHeight;
    }

    std::map<entt::entity, UIRect> empty;
    // Calculate in local canvas space (0,0 relative)
    UIRect virtualCanvas = {
        0,
        0,
        viewportSize.x / scaleFactor,
        viewportSize.y / scaleFactor,
    };

    UIRect virtualRect = CalculateEntityRect(entity, virtualCanvas, empty);
    return {
        viewportPos.x + virtualRect.x * scaleFactor,
        viewportPos.y + virtualRect.y * scaleFactor,
        virtualRect.width * scaleFactor,
        virtualRect.height * scaleFactor,
    };
}

// ---------------------------------------------------------------------------
// Private Helpers
// ---------------------------------------------------------------------------

UIRect UIRenderer::CalculateEntityRect(Entity entity, const UIRect& canvasRect, std::map<entt::entity, UIRect>& cache)
{
    entt::entity id = (entt::entity)entity;
    if (cache.count(id))
    {
        return cache[id];
    }

    auto& control = entity.GetComponent<ControlComponent>();
    UIRect parentRect = canvasRect;

    if (entity.HasComponent<HierarchyComponent>())
    {
        auto parentID = entity.GetComponent<HierarchyComponent>().Parent;
        if (parentID != entt::null)
        {
            Entity parent(parentID, entity.GetRegistryPtr());
            if (parent.HasComponent<ControlComponent>())
            {
                parentRect = CalculateEntityRect(parent, canvasRect, cache);
            }
        }
    }

    // Auto-size logic for labels
    if (entity.HasComponent<WidgetComponent>())
    {
        auto& widget = entity.GetComponent<WidgetComponent>();
        if (std::holds_alternative<LabelData>(widget.Data))
        {
            auto& label = std::get<LabelData>(widget.Data);
            if (label.AutoSize)
            {
                ImVec2 ts = ImGui::CalcTextSize(label.Text.c_str());
                control.Transform.OffsetMax = {control.Transform.OffsetMin.x + ts.x + 10.0f,
                                               control.Transform.OffsetMin.y + ts.y + 4.0f};
            }
        }
    }

    Rectangle r = ComponentUtils::CalculateRect(control.Transform, {parentRect.width, parentRect.height},
                                                {parentRect.x, parentRect.y});
    UIRect result = {r.x, r.y, r.width, r.height};
    cache[id] = result;
    return result;
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

void UIRenderer::UpdateStyleAnimation(UIStyle& style, bool isHovered, bool isDown, float dt)
{
    float target = isDown ? 1.0f : (isHovered ? 0.5f : 0.0f);

    if (style.TransitionSpeed > 0.0f)
    {
        style.State.AnimationAlpha +=
            (target - style.State.AnimationAlpha) * std::min(1.0f, dt / style.TransitionSpeed);
    }
    else
    {
        style.State.AnimationAlpha = target;
    }

    float a = style.State.AnimationAlpha;
    style.State.CurrentScale =
        1.0f + (isDown ? (style.PressedScale - 1.0f) : (isHovered ? (style.HoverScale - 1.0f) : 0.0f)) * a;

    auto LerpColor = [](Color c1, Color c2, float t) -> Color {
        return {(uint8_t)(c1.r + (c2.r - c1.r) * t), (uint8_t)(c1.g + (c2.g - c1.g) * t),
                (uint8_t)(c1.b + (c2.b - c1.b) * t), (uint8_t)(c1.a + (c2.a - c1.a) * t)};
    };

    style.State.CurrentColor = (a <= 0.5f) ? LerpColor(style.BackgroundColor, style.HoverColor, a * 2.0f)
                                           : LerpColor(style.HoverColor, style.PressedColor, (a - 0.5f) * 2.0f);
}

// ---------------------------------------------------------------------------
// RenderUIComponent — dispatcher, delegates to UI::Render* free functions
// ---------------------------------------------------------------------------

bool UIRenderer::RenderUIComponent(Entity entity, const ImVec2& screenPos, const ImVec2& size, bool editMode)
{
    if (!entity.HasComponent<WidgetComponent>())
        return false;

    auto& widget = entity.GetComponent<WidgetComponent>();
    return UI::Dispatcher::Render(entity, widget, screenPos, size);
}

void UIRenderer::ResetButtonStates(Scene* scene)
{
    if (!scene)
    {
        return;
    }
    auto& registry = scene->GetRegistry();

    auto view = registry.view<WidgetComponent>();
    for (entt::entity id : view)
    {
        view.get<WidgetComponent>(id).PressedThisFrame = false;
    }
}

void UIRenderer::DrawCanvas(Scene* scene, const ImVec2& referencePosition, const ImVec2& referenceSize, bool editMode)
{
    CH_CORE_ASSERT(scene, "Scene is null!");

    ImVec2 refSize = (referenceSize.x > 0) ? referenceSize : ImGui::GetIO().DisplaySize;
    if (refSize.x <= 0 || refSize.y <= 0)
    {
        return;
    }
    auto& registry = scene->GetRegistry();

    auto uiEntities = SortUIEntities(registry);

    if (m_InputCooldownFrames > 0)
    {
        m_InputCooldownFrames--;
    }

    const int frameNumber = ImGui::GetFrameCount();
    static int s_LastResetFrame = -1;
    static void* s_LastResetRegistry = nullptr;
    
    if (s_LastResetFrame != frameNumber || s_LastResetRegistry != (void*)&registry)
    {
        // Reset button states at the start of the frame for this registry
        auto view = registry.view<WidgetComponent>();
        for (entt::entity id : view)
        {
            view.get<WidgetComponent>(id).PressedThisFrame = false;
        }

        s_LastResetFrame = frameNumber;
        s_LastResetRegistry = (void*)&registry;
    }

    const CanvasSettings& canvas = scene->GetSettings().Canvas;
    float scaleFactor = 1.0f;

    if (canvas.ScaleMode == CanvasScaleMode::ScaleWithScreenSize && canvas.ReferenceResolution.x > 0 &&
        canvas.ReferenceResolution.y > 0)
    {
        float scaleX = refSize.x / canvas.ReferenceResolution.x;
        float scaleY = refSize.y / canvas.ReferenceResolution.y;
        scaleFactor = scaleX * (1.0f - canvas.MatchWidthOrHeight) + scaleY * canvas.MatchWidthOrHeight;
    }

    float virtualW = refSize.x / scaleFactor;
    float virtualH = refSize.y / scaleFactor;

    // Canvas space is relative to (0,0) internally for recursion, 
    // we add the screen offset (referencePosition) at the final step.
    UIRect canvasRect = {0, 0, virtualW, virtualH};

    ImVec2 canvasClipMin = referencePosition;
    ImVec2 canvasClipMax = {referencePosition.x + refSize.x, referencePosition.y + refSize.y};
    ImGui::PushClipRect(canvasClipMin, canvasClipMax, true);

    std::map<entt::entity, UIRect> rectCache;
    float dt = ImGui::GetIO().DeltaTime;

    for (entt::entity id : uiEntities)
    {
        Entity entity(id, &registry);
        auto& control = registry.get<ControlComponent>(id);
        if (!control.IsActive)
        {
            continue;
        }

        if (entity.HasComponent<WidgetComponent>())
        {
            auto& widget = entity.GetComponent<WidgetComponent>();
            UpdateStyleAnimation(widget.BoxStyle, widget.IsHovered, widget.IsDown, dt);
        }

        UIRect rect = CalculateEntityRect(entity, canvasRect, rectCache);
        ImVec2 screenPos = {referencePosition.x + rect.x * scaleFactor, 
                            referencePosition.y + rect.y * scaleFactor};
        ImVec2 size = {rect.width * scaleFactor, rect.height * scaleFactor};

        bool needsClipPop = false;
        if (entity.HasComponent<HierarchyComponent>())
        {
            auto parentID = entity.GetComponent<HierarchyComponent>().Parent;
            if (parentID != entt::null && rectCache.count(parentID))
            {
                UIRect parentRect = rectCache[parentID];
                if (parentRect.width > 1.0f && parentRect.height > 1.0f)
                {
                    ImVec2 clipMin = {referencePosition.x + parentRect.x * scaleFactor, 
                                      referencePosition.y + parentRect.y * scaleFactor};
                    ImVec2 clipMax = {referencePosition.x + (parentRect.x + parentRect.width) * scaleFactor,
                                      referencePosition.y + (parentRect.y + parentRect.height) * scaleFactor};
                    ImGui::PushClipRect(clipMin, clipMax, true);
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
                        CH_CORE_WARN("[UI] Entity '{}' has WidgetComponent with empty WidgetData (monostate)",
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

        if (needsClipPop)
        {
            ImGui::PopClipRect();
        }
    }

    ImGui::PopClipRect();
}

} // namespace CHEngine
