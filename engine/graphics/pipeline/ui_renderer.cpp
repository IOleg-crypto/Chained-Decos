#include "ui_renderer.h"
#include "ui_widget_renderer.h"
#include "engine/core/profiler.h"
#include "engine/assets/asset_manager.h"
#include "engine/core/service_locator.h"
#include "engine/graphics/loaders/font_loader.h"
#include "engine/scene/components/component_utils.h"
#include "engine/scene/scene.h"
#include <algorithm>
#include <map>

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
    if (m_Initialized)
    {
        CH_CORE_INFO("Shutting down UIRenderer...");
        m_Initialized = false;
    }
}


void UIRenderer::LoadProjectFonts()
{
    m_FontRegistry.Clear();
    m_FontRegistry.LoadProjectFonts();
}

// ---------------------------------------------------------------------------
// Animation
// ---------------------------------------------------------------------------

static Color LerpColor(const Color& a, const Color& b, float t)
{
    return {
        (uint8_t)(a.r + (b.r - a.r) * t),
        (uint8_t)(a.g + (b.g - a.g) * t),
        (uint8_t)(a.b + (b.b - a.b) * t),
        (uint8_t)(a.a + (b.a - a.a) * t)
    };
}

void UIRenderer::UpdateStyleAnimation(UIStyle& style, bool isHovered, bool isDown, float dt)
{
    float target = isDown ? 1.0f : (isHovered ? 0.5f : 0.0f);
    if (style.TransitionSpeed > 0.0f)
        style.State.AnimationAlpha += (target - style.State.AnimationAlpha) * std::min(1.0f, dt / style.TransitionSpeed);
    else
        style.State.AnimationAlpha = target;

    float a = style.State.AnimationAlpha;
    style.State.CurrentColor = (a <= 0.5f)
        ? LerpColor(style.BackgroundColor, style.HoverColor,    a * 2.0f)
        : LerpColor(style.HoverColor,      style.PressedColor,  (a - 0.5f) * 2.0f);

    float targetScale = isDown ? style.PressedScale : (isHovered ? style.HoverScale : 1.0f);
    style.State.CurrentScale = 1.0f + (targetScale - 1.0f) * a;
}

// ---------------------------------------------------------------------------
// Layout helpers
// ---------------------------------------------------------------------------

std::vector<entt::entity> UIRenderer::SortUIEntities(entt::registry& registry)
{
    auto view = registry.view<ControlComponent>();
    std::vector<entt::entity> sorted(view.begin(), view.end());
    std::sort(sorted.begin(), sorted.end(), [&](entt::entity a, entt::entity b) {
        return view.get<ControlComponent>(a).ZOrder < view.get<ControlComponent>(b).ZOrder;
    });
    return sorted;
}

UIRect UIRenderer::CalculateEntityRect(Entity entity, const UIRect& canvasRect, std::map<entt::entity, UIRect>& cache)
{
    auto& control = entity.GetComponent<ControlComponent>();

    UIRect parentRect = canvasRect;
    if (entity.HasComponent<HierarchyComponent>())
    {
        auto parentID = entity.GetComponent<HierarchyComponent>().Parent;
        if (parentID != entt::null)
        {
            if (cache.count(parentID))
            {
                parentRect = cache[parentID];
            }
            else
            {
                Entity parentEntity{parentID, entity.GetRegistryPtr()};
                if (parentEntity.HasComponent<ControlComponent>())
                {
                    parentRect = CalculateEntityRect(parentEntity, canvasRect, cache);
                    cache[parentID] = parentRect;
                }
            }
        }
    }

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
        else if (std::holds_alternative<ButtonData>(widget.Data))
        {
            auto& btn = std::get<ButtonData>(widget.Data);
            if (btn.AutoSize)
            {
                ImVec2 ts = ImGui::CalcTextSize(btn.Label.c_str());
                float pad = widget.BoxStyle.Padding * 2.0f;
                control.Transform.OffsetMax = {control.Transform.OffsetMin.x + ts.x + pad + 10.0f,
                                               control.Transform.OffsetMin.y + ts.y + pad + 4.0f};
            }
        }
    }

    Rectangle r = ComponentUtils::CalculateRect(control.Transform, {parentRect.width, parentRect.height}, {parentRect.x, parentRect.y});
    return {r.x, r.y, r.width, r.height};
}

UIRect UIRenderer::GetEntityRect(Entity entity, const ImVec2& viewportSize, const ImVec2& viewportPos)
{
    if (!entity || !entity.HasComponent<ControlComponent>())
    {
        return {0, 0, 0, 0};
    }

    float scaleFactor = 1.0f;
    auto* sceneCtx = entity.GetRegistry().ctx().find<Scene*>();
    if (sceneCtx && *sceneCtx)
    {
        const CanvasSettings& canvasSettings = (*sceneCtx)->GetSettings().Canvas;
        if (canvasSettings.ScaleMode == CanvasScaleMode::ScaleWithScreenSize &&
            canvasSettings.ReferenceResolution.x > 0.0f && canvasSettings.ReferenceResolution.y > 0.0f)
        {
            const float scaleX = viewportSize.x / canvasSettings.ReferenceResolution.x;
            const float scaleY = viewportSize.y / canvasSettings.ReferenceResolution.y;
            scaleFactor = scaleX * (1.0f - canvasSettings.MatchWidthOrHeight) +
                          scaleY * canvasSettings.MatchWidthOrHeight;

            if (scaleFactor <= 0.0001f)
            {
                scaleFactor = 1.0f;
            }
        }
    }

    std::map<entt::entity, UIRect> empty;
    UIRect virtualCanvas = {
        viewportPos.x / scaleFactor,
        viewportPos.y / scaleFactor,
        viewportSize.x / scaleFactor,
        viewportSize.y / scaleFactor,
    };

    UIRect virtualRect = CalculateEntityRect(entity, virtualCanvas, empty);
    return {
        virtualRect.x * scaleFactor,
        virtualRect.y * scaleFactor,
        virtualRect.width * scaleFactor,
        virtualRect.height * scaleFactor,
    };
}

// ---------------------------------------------------------------------------
// RenderUIComponent — dispatcher, delegates to UI::Render* free functions
// ---------------------------------------------------------------------------

bool UIRenderer::RenderUIComponent(Entity entity, const ImVec2& screenPos, const ImVec2& size, bool editMode)
{
    bool handled = false;
    auto& reg    = entity.GetRegistry();

    if (entity.HasComponent<WidgetComponent>())
    {
        auto& widget = entity.GetComponent<WidgetComponent>();
        
        UI::StyleCounts styleState = UI::PushUIStyle(widget.BoxStyle);
        UI::PushTextStyle(widget.TextStyle, styleState);

        std::visit([&](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, std::monostate>) {
                // Do nothing
            }
            else if constexpr (std::is_same_v<T, PanelData>) {
                UI::RenderPanel(arg, widget, screenPos, size);
            }
            else if constexpr (std::is_same_v<T, LabelData>) {
                UI::RenderLabel(arg, widget, size);
            }
            else if constexpr (std::is_same_v<T, ButtonData>) {
                handled = UI::RenderButton(entity, arg, widget, size);
            }
            else if constexpr (std::is_same_v<T, SliderData>) {
                handled = UI::RenderSlider(arg, widget, size);
            }
            else if constexpr (std::is_same_v<T, CheckboxData>) {
                handled = UI::RenderCheckbox(arg, widget);
            }
            else if constexpr (std::is_same_v<T, ImageData>) {
                UI::RenderImage(arg, widget, size);
            }
            else if constexpr (std::is_same_v<T, InputTextData>) {
                handled = UI::RenderInputText(entity, arg, widget, size);
            }
            else if constexpr (std::is_same_v<T, ProgressBarData>) {
                UI::RenderProgressBar(arg, widget, size);
            }
            else if constexpr (std::is_same_v<T, ComboBoxData>) {
                handled = UI::RenderComboBox(arg, widget, size);
            }
            else if constexpr (std::is_same_v<T, ImageButtonData>) {
                handled = UI::RenderImageButton(arg, widget, size);
            }
            else if constexpr (std::is_same_v<T, RadioButtonData>) {
                handled = UI::RenderRadioButton(arg, widget);
            }
            else if constexpr (std::is_same_v<T, ColorPickerData>) {
                handled = UI::RenderColorPicker(arg, widget);
            }
            else if constexpr (std::is_same_v<T, SeparatorData>) {
                UI::RenderSeparator(arg);
            }
            else if constexpr (std::is_same_v<T, DragFloatData>) {
                handled = UI::RenderDragFloat(arg, widget, size);
            }
            else if constexpr (std::is_same_v<T, DragIntData>) {
                handled = UI::RenderDragInt(arg, widget, size);
            }
            else if constexpr (std::is_same_v<T, TreeNodeData>) {
                handled = UI::RenderTreeNode(arg, widget);
            }
            else if constexpr (std::is_same_v<T, CollapsingHeaderData>) {
                handled = UI::RenderCollapsingHeader(arg, widget);
            }
            else if constexpr (std::is_same_v<T, PlotLinesData>) {
                handled = UI::RenderPlotLines(arg, widget);
            }
            else if constexpr (std::is_same_v<T, PlotHistogramData>) {
                handled = UI::RenderPlotHistogram(arg, widget);
            }
            else if constexpr (std::is_same_v<T, TabBarData>) {
                UI::RenderTabBar(entity, arg, widget, reg);
                handled = true;
            }
        }, widget.Data);

        UI::PopUIStyle(styleState);
    }

    return handled;
}

// ---------------------------------------------------------------------------
// ResetButtonStates — resets per-frame one-shot flags before scripts run
// ---------------------------------------------------------------------------

void UIRenderer::ResetButtonStates(Scene* scene)
{
    if (!scene) return;
    auto& registry = scene->GetRegistry();

    auto view = registry.view<WidgetComponent>();
    for (entt::entity id : view)
        view.get<WidgetComponent>(id).PressedThisFrame = false;
}

// ---------------------------------------------------------------------------
// DrawCanvas — main entry point
// ---------------------------------------------------------------------------

void UIRenderer::DrawCanvas(Scene* scene, const ImVec2& referencePosition, const ImVec2& referenceSize, bool editMode)
{
    CH_CORE_ASSERT(scene, "Scene is null!");
    CH_PROFILE_FUNCTION();

    ImVec2 refSize = (referenceSize.x > 0) ? referenceSize : ImGui::GetIO().DisplaySize;
    if (refSize.x <= 0 || refSize.y <= 0)
        return;
    auto&  registry  = scene->GetRegistry();

    auto uiEntities = SortUIEntities(registry);

    // Reset one-shot button press flags once per ImGui frame per registry.
    const int frameNumber = ImGui::GetFrameCount();
    static int s_LastResetFrame = -1;
    static const entt::registry* s_LastResetRegistry = nullptr;
    if (s_LastResetFrame != frameNumber || s_LastResetRegistry != &registry)
    {
        auto view = registry.view<WidgetComponent>();
        for (entt::entity id : view)
            view.get<WidgetComponent>(id).PressedThisFrame = false;

        s_LastResetFrame = frameNumber;
        s_LastResetRegistry = &registry;
    }

    const CanvasSettings& canvas = scene->GetSettings().Canvas;
    float scaleFactor = 1.0f;
    if (canvas.ScaleMode == CanvasScaleMode::ScaleWithScreenSize &&
        canvas.ReferenceResolution.x > 0 && canvas.ReferenceResolution.y > 0)
    {
        float scaleX = refSize.x / canvas.ReferenceResolution.x;
        float scaleY = refSize.y / canvas.ReferenceResolution.y;
        scaleFactor  = scaleX * (1.0f - canvas.MatchWidthOrHeight) + scaleY * canvas.MatchWidthOrHeight;
    }

    float virtualW  = refSize.x / scaleFactor;
    float virtualH  = refSize.y / scaleFactor;
    float virtualOX = referencePosition.x / scaleFactor;
    float virtualOY = referencePosition.y / scaleFactor;

    UIRect canvasRect = {virtualOX, virtualOY, virtualW, virtualH};
    std::map<entt::entity, UIRect> rectCache;
    float dt = ImGui::GetIO().DeltaTime;

    // Clip all UI rendering to canvas bounds first.
    ImVec2 canvasClipMin = referencePosition;
    ImVec2 canvasClipMax = {referencePosition.x + refSize.x, referencePosition.y + refSize.y};
    ImGui::PushClipRect(canvasClipMin, canvasClipMax, true);

    for (entt::entity id : SortUIEntities(registry))
    {
        Entity entity(id, &registry);
        auto& control = registry.get<ControlComponent>(id);
        if (!control.IsActive) continue;

        // Animate interactable controls
        if (entity.HasComponent<WidgetComponent>()) {
            auto& widget = entity.GetComponent<WidgetComponent>();
            UpdateStyleAnimation(widget.BoxStyle, widget.IsHovered, widget.IsDown, dt);
        }

        UIRect rect = CalculateEntityRect(entity, canvasRect, rectCache);
        rectCache[id] = rect;

        ImVec2 screenPos = {rect.x * scaleFactor, rect.y * scaleFactor};
        ImVec2 size      = {rect.width * scaleFactor, rect.height * scaleFactor};

        // Apply clipping for child elements to their parent bounds
        bool needsClipPop = false;
        if (entity.HasComponent<HierarchyComponent>())
        {
            auto parentID = entity.GetComponent<HierarchyComponent>().Parent;
            if (parentID != entt::null && registry.valid(parentID))
            {
                UIRect parentRect{};
                bool hasParentRect = false;

                auto it = rectCache.find(parentID);
                if (it != rectCache.end())
                {
                    parentRect = it->second;
                    hasParentRect = true;
                }
                else
                {
                    Entity parentEntity{parentID, &registry};
                    if (parentEntity.HasComponent<ControlComponent>())
                    {
                        parentRect = CalculateEntityRect(parentEntity, canvasRect, rectCache);
                        rectCache[parentID] = parentRect;
                        hasParentRect = true;
                    }
                }

                if (hasParentRect && parentRect.width > 1.0f && parentRect.height > 1.0f)
                {
                    ImVec2 clipMin = {parentRect.x * scaleFactor, parentRect.y * scaleFactor};
                    ImVec2 clipMax = {(parentRect.x + parentRect.width) * scaleFactor,
                                      (parentRect.y + parentRect.height) * scaleFactor};
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

            RenderUIComponent(entity, screenPos, size, editMode);

            if (editMode)
            {
                ImGui::SetCursorScreenPos(screenPos);
                ImGui::InvisibleButton("##DragZone", size);

                if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
                {
                    ImVec2 delta = ImGui::GetIO().MouseDelta;
                    control.Transform.OffsetMin.x += delta.x / scaleFactor;
                    control.Transform.OffsetMax.x += delta.x / scaleFactor;
                    control.Transform.OffsetMin.y += delta.y / scaleFactor;
                    control.Transform.OffsetMax.y += delta.y / scaleFactor;
                }
            }

            ImGui::PopID();
            ImGui::EndGroup();
        }

        if (needsClipPop)
            ImGui::PopClipRect();
    }

    ImGui::PopClipRect();
}

} // namespace CHEngine
