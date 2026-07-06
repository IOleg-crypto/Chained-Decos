#include "ui_renderer.h"
#include "engine/core/service_locator.h"
#include <algorithm>

#include "engine/app/application.h"
#include "engine/core/log.h"
#include "engine/scene/components/component_utils.h"
#include "engine/scene/components/hierarchy_component.h"
#include "engine/scene/components/tag_component.h"
#include "engine/scene/scene.h"
#include "ui_control_renderer.h"

namespace Chained {

UIRenderer::UIRenderer() {}

void UIRenderer::Initialize() {
    CH_CORE_INFO("[UI] Initializing UI Renderer...");
    m_Initialized = true;
}

void UIRenderer::Shutdown() {}

void UIRenderer::Update(Timestep ts) {}

void UIRenderer::LoadProjectFonts() {
    m_FontRegistry.LoadProjectFonts();
}

UIRect UIRenderer::GetEntityRect(Scene *scene, Entity entity, const ImVec2 &viewportSize, const ImVec2 &viewportPos) {
    // Оскільки оператор приведення або метод доступу до дескриптора в Entity не явний,
    // для внутрішніх розрахунків ми створюємо/використовуємо сутність через оператор приведення,
    // або якщо в Entity є implicit cast до entt::entity, використовуємо його:
    return m_LayoutSystem.GetEntityRect((entt::entity)entity);
}

void UIRenderer::ResetButtonStates(Scene *scene) {
    if (!scene)
        return;

    auto &registry = scene->GetRegistry();
    auto view = registry.view<UIControlComponent>();
    for (entt::entity id : view) {
        view.get<UIControlComponent>(id).PressedThisFrame = false;
    }
}

std::vector<entt::entity> UIRenderer::SortUIEntities(entt::registry &registry) {
    auto view = registry.view<ControlComponent>();
    std::vector<entt::entity> sorted(view.begin(), view.end());

    // Сортування за Z-порядком для правильної послідовності відрисування (back-to-front)
    std::sort(sorted.begin(), sorted.end(), [&](entt::entity a, entt::entity b) {
        return view.get<ControlComponent>(a).ZOrder < view.get<ControlComponent>(b).ZOrder;
    });

    return sorted;
}

bool UIRenderer::RenderUIComponent(Entity entity, const ImVec2 &screenPos, const ImVec2 &size, bool editMode) {
    if (!entity.HasComponent<UIControlComponent>())
        return false;

    auto &control = entity.GetComponent<UIControlComponent>();
    // Виклик низькорівневого рендерера на базі ImDrawList
    return RenderControl(m_FontRegistry, entity, control, screenPos, size);
}

void UIRenderer::DrawCanvas(Scene *scene, const ImVec2 &referencePosition, const ImVec2 &referenceSize, bool editMode) {
    CH_CORE_ASSERT(scene, "Scene is null!");

    ImVec2 refSize = (referenceSize.x > 0) ? referenceSize : ImGui::GetIO().DisplaySize;
    if (refSize.x <= 0 || refSize.y <= 0)
        return;

    auto &registry = scene->GetRegistry();

    // 1. Оновлення та синхронізація внутрішніх UI систем
    m_LayoutSystem.Update(scene, refSize, referencePosition);

    if (m_InputCooldownFrames > 0)
        m_InputCooldownFrames--;

    m_InputSystem.Update(registry, m_LayoutSystem, m_InputCooldownFrames);
    m_AnimationSystem.Update(registry, ImGui::GetIO().DeltaTime);

    // 2. Рендеринг елементів
    auto uiEntities = SortUIEntities(registry);

    ImVec2 canvasClipMin = referencePosition;
    ImVec2 canvasClipMax = {referencePosition.x + refSize.x, referencePosition.y + refSize.y};
    ImGui::PushClipRect(canvasClipMin, canvasClipMax, true);

    for (entt::entity id : uiEntities) {
        // ФІКС 1: Передаємо вказівник на entt::registry, як того вимагає конструктор у вашому entity.cpp
        Entity entity(id, scene->GetRegistryPtr());

        auto &control = registry.get<ControlComponent>(id);
        if (!control.IsActive)
            continue;

        // ФІКС 2: Замість entity.GetHandle() використовуємо локальний `id`, який вже є типу entt::entity
        UIRect rect = m_LayoutSystem.GetEntityRect(id);
        ImVec2 screenPos = {rect.x, rect.y};
        ImVec2 size = {rect.width, rect.height};

        bool needsClipPop = false;

        // Безпечне отримання ієрархії батьківського елемента для відсікання (Scissor Test)
        if (entity.HasComponent<HierarchyComponent>()) {
            auto &hierarchy = entity.GetComponent<HierarchyComponent>();
            auto parentID = hierarchy.Parent;

            if (parentID != entt::null) {
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

            // Спроба відрисувати компонент контролю
            if (!RenderUIComponent(entity, screenPos, size, editMode)) {
                if (entity.HasComponent<UIControlComponent>()) {
                    auto &widget = entity.GetComponent<UIControlComponent>();
                    if (std::holds_alternative<std::monostate>(widget.Data)) {
                        CH_CORE_WARN("[UI] Entity '{}' has UIControlComponent with empty ControlData",
                                     entity.GetComponent<TagComponent>().Tag);
                    }
                }
            }

            // В режимі редагування перекриваємо зону невидимою кнопкою, щоб не спрацьовувала логіка гри
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