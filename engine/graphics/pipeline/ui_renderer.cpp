#include "ui_renderer.h"
#include "ui_widget_renderer.h"
#include "engine/core/profiler.h"
#include "engine/scene/components.h"
#include "engine/scene/project.h"
#include "engine/scene/scene.h"
#include <algorithm>
#include <map>

namespace CHEngine
{

UIRenderer* UIRenderer::s_Instance = nullptr;

UIRenderer& UIRenderer::Get()
{
    CH_CORE_ASSERT(s_Instance, "UIRenderer not initialized!");
    return *s_Instance;
}

void UIRenderer::Init()
{
    if (!s_Instance)
    {
        s_Instance = new UIRenderer();
    }

    if (s_Instance->m_Initialized)
    {
        return;
    }

    s_Instance->m_Initialized = true;
    CH_CORE_INFO("Initializing UIRenderer...");
}

void UIRenderer::Shutdown()
{
    if (s_Instance)
    {
        if (s_Instance->m_Initialized)
        {
            CH_CORE_INFO("Shutting down UIRenderer...");
            s_Instance->m_Initialized = false;
        }
        delete s_Instance;
        s_Instance = nullptr;
    }
}

UIRenderer::UIRenderer()  {}
UIRenderer::~UIRenderer() {}

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
                Entity parentEntity{parentID, entity.GetRegistry()};
                if (parentEntity.HasComponent<ControlComponent>())
                {
                    parentRect = CalculateEntityRect(parentEntity, canvasRect, cache);
                    cache[parentID] = parentRect;
                }
            }
        }
    }

    // AutoSize
    if (entity.HasComponent<LabelControl>() && entity.GetComponent<LabelControl>().AutoSize)
    {
        auto& label  = entity.GetComponent<LabelControl>();
        ImVec2 ts    = ImGui::CalcTextSize(label.Text.c_str());
        control.Transform.OffsetMax = {control.Transform.OffsetMin.x + ts.x + 10.0f,
                                       control.Transform.OffsetMin.y + ts.y + 4.0f};
    }
    else if (entity.HasComponent<ButtonControl>() && entity.GetComponent<ButtonControl>().AutoSize)
    {
        auto& btn  = entity.GetComponent<ButtonControl>();
        ImVec2 ts  = ImGui::CalcTextSize(btn.Label.c_str());
        float pad  = btn.Style.Padding * 2.0f;
        control.Transform.OffsetMax = {control.Transform.OffsetMin.x + ts.x + pad + 10.0f,
                                       control.Transform.OffsetMin.y + ts.y + pad + 4.0f};
    }

    Rectangle r = control.Transform.CalculateRect({parentRect.width, parentRect.height}, {parentRect.x, parentRect.y});
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

    if (entity.HasComponent<PanelControl>())
        UI::RenderPanel(entity.GetComponent<PanelControl>(), screenPos, size);

    if (entity.HasComponent<LabelControl>())
        UI::RenderLabel(entity.GetComponent<LabelControl>(), size);

    if (entity.HasComponent<ButtonControl>())
        handled |= UI::RenderButton(entity, entity.GetComponent<ButtonControl>(), size);

    if (entity.HasComponent<SliderControl>())
        handled |= UI::RenderSlider(entity.GetComponent<SliderControl>(), size);

    if (entity.HasComponent<CheckboxControl>())
        handled |= UI::RenderCheckbox(entity.GetComponent<CheckboxControl>());

    if (entity.HasComponent<ImageControl>())
        UI::RenderImage(entity.GetComponent<ImageControl>(), size);

    if (entity.HasComponent<InputTextControl>())
        handled |= UI::RenderInputText(entity, entity.GetComponent<InputTextControl>(), size);

    if (entity.HasComponent<ProgressBarControl>())
        UI::RenderProgressBar(entity.GetComponent<ProgressBarControl>(), size);

    if (entity.HasComponent<ComboBoxControl>())
        handled |= UI::RenderComboBox(entity.GetComponent<ComboBoxControl>(), size);

    if (entity.HasComponent<ImageButtonControl>())
        handled |= UI::RenderImageButton(entity.GetComponent<ImageButtonControl>(), size);

    if (entity.HasComponent<RadioButtonControl>())
        handled |= UI::RenderRadioButton(entity.GetComponent<RadioButtonControl>());

    if (entity.HasComponent<ColorPickerControl>())
        handled |= UI::RenderColorPicker(entity.GetComponent<ColorPickerControl>());

    if (entity.HasComponent<SeparatorControl>())
        UI::RenderSeparator(entity.GetComponent<SeparatorControl>());

    if (entity.HasComponent<DragFloatControl>())
        handled |= UI::RenderDragFloat(entity.GetComponent<DragFloatControl>(), size);

    if (entity.HasComponent<DragIntControl>())
        handled |= UI::RenderDragInt(entity.GetComponent<DragIntControl>(), size);

    if (entity.HasComponent<TreeNodeControl>())
        handled |= UI::RenderTreeNode(entity.GetComponent<TreeNodeControl>());

    if (entity.HasComponent<CollapsingHeaderControl>())
        handled |= UI::RenderCollapsingHeader(entity.GetComponent<CollapsingHeaderControl>());

    if (entity.HasComponent<PlotLinesControl>())
        handled |= UI::RenderPlotLines(entity.GetComponent<PlotLinesControl>());

    if (entity.HasComponent<PlotHistogramControl>())
        handled |= UI::RenderPlotHistogram(entity.GetComponent<PlotHistogramControl>());

    if (entity.HasComponent<TabBarControl>())
    {
        UI::RenderTabBar(entity, entity.GetComponent<TabBarControl>(), reg);
        handled = true;
    }

    return handled;
}

// ---------------------------------------------------------------------------
// DrawCanvas — main entry point
// ---------------------------------------------------------------------------

void UIRenderer::DrawCanvas(Scene* scene, const ImVec2& referencePosition, const ImVec2& referenceSize, bool editMode)
{
    CH_CORE_ASSERT(scene, "Scene is null!");
    CH_PROFILE_FUNCTION();

    ImVec2 refSize   = (referenceSize.x > 0) ? referenceSize : ImGui::GetIO().DisplaySize;
    auto&  registry  = scene->GetRegistry();

    // Reset one-shot button press flags once per ImGui frame per registry.
    const int frameNumber = ImGui::GetFrameCount();
    static int s_LastResetFrame = -1;
    static const entt::registry* s_LastResetRegistry = nullptr;
    if (s_LastResetFrame != frameNumber || s_LastResetRegistry != &registry)
    {
        auto buttonView = registry.view<ButtonControl>();
        for (entt::entity id : buttonView)
        {
            buttonView.get<ButtonControl>(id).PressedThisFrame = false;
        }

        auto imageButtonView = registry.view<ImageButtonControl>();
        for (entt::entity id : imageButtonView)
        {
            imageButtonView.get<ImageButtonControl>(id).PressedThisFrame = false;
        }

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
        if (entity.HasComponent<ButtonControl>()) {
            auto& btn = entity.GetComponent<ButtonControl>();
            UpdateStyleAnimation(btn.Style, btn.IsHovered, btn.IsDown, dt);
        }
        if (entity.HasComponent<ImageControl>()) {
            auto& img = entity.GetComponent<ImageControl>();
            UpdateStyleAnimation(img.Style, img.IsHovered, img.IsDown, dt);
        }
        if (entity.HasComponent<PanelControl>()) {
            auto& panel = entity.GetComponent<PanelControl>();
            UpdateStyleAnimation(panel.Style, panel.IsHovered, panel.IsDown, dt);
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
