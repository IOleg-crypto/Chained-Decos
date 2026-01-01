#include "ScriptGlue.h"
#include "DotNetHost.h"
#include "core/Log.h"
#include "core/scripting/ScriptManager.h"
#include "scene/ecs/ECSRegistry.h"
#include "scene/ecs/components/TransformComponent.h"
#include "scene/ecs/components/UIComponents.h"
#include <imgui.h>
#include <raylib.h>

namespace CHEngine
{
// Helper to convert managed string to C++ string (assuming UTF-16)
// For simplicity, we'll implement this later with proper interop.

static ScriptGlue::ScriptDelegates s_Delegates;

ScriptGlue::ScriptDelegates &ScriptGlue::GetDelegates()
{
    return s_Delegates;
}

void ScriptGlue::Log_Info(void *message)
{
    // Simple placeholder for message extraction
    CD_INFO("[C#] %ls", (const wchar_t *)message);
}

void ScriptGlue::Log_Warning(void *message)
{
    CD_WARN("[C#] %ls", (const wchar_t *)message);
}

void ScriptGlue::Log_Error(void *message)
{
    CD_ERROR("[C#] %ls", (const wchar_t *)message);
}

void ScriptGlue::Transform_GetPosition(uint32_t entityID, void *outPos)
{
    entt::registry *registry = ScriptManager::GetActiveRegistry();
    if (!registry)
        return;

    entt::entity entity = (entt::entity)entityID;
    if (registry->all_of<TransformComponent>(entity))
    {
        *(Vector3 *)outPos = registry->get<TransformComponent>(entity).position;
    }
}

void ScriptGlue::Transform_SetPosition(uint32_t entityID, void *inPos)
{
    entt::registry *registry = ScriptManager::GetActiveRegistry();
    if (!registry)
        return;

    entt::entity entity = (entt::entity)entityID;
    if (registry->all_of<TransformComponent>(entity))
    {
        registry->get<TransformComponent>(entity).position = *(Vector3 *)inPos;
    }
}

// --- RectTransform ---
void ScriptGlue::RectTransform_GetPosition(uint32_t entityID, void *outPos)
{
    auto *registry = ScriptManager::GetActiveRegistry();
    if (registry && registry->all_of<RectTransform>((entt::entity)entityID))
        *(Vector2 *)outPos = registry->get<RectTransform>((entt::entity)entityID).position;
}

void ScriptGlue::RectTransform_SetPosition(uint32_t entityID, void *inPos)
{
    auto *registry = ScriptManager::GetActiveRegistry();
    if (registry && registry->all_of<RectTransform>((entt::entity)entityID))
        registry->get<RectTransform>((entt::entity)entityID).position = *(Vector2 *)inPos;
}

void ScriptGlue::RectTransform_GetSize(uint32_t entityID, void *outSize)
{
    auto *registry = ScriptManager::GetActiveRegistry();
    if (registry && registry->all_of<RectTransform>((entt::entity)entityID))
        *(Vector2 *)outSize = registry->get<RectTransform>((entt::entity)entityID).size;
}

void ScriptGlue::RectTransform_SetSize(uint32_t entityID, void *inSize)
{
    auto *registry = ScriptManager::GetActiveRegistry();
    if (registry && registry->all_of<RectTransform>((entt::entity)entityID))
        registry->get<RectTransform>((entt::entity)entityID).size = *(Vector2 *)inSize;
}

uint8_t ScriptGlue::RectTransform_GetAnchor(uint32_t entityID)
{
    auto *registry = ScriptManager::GetActiveRegistry();
    if (registry && registry->all_of<RectTransform>((entt::entity)entityID))
        return (uint8_t)registry->get<RectTransform>((entt::entity)entityID).anchor;
    return 0;
}

void ScriptGlue::RectTransform_SetAnchor(uint32_t entityID, uint8_t anchor)
{
    auto *registry = ScriptManager::GetActiveRegistry();
    if (registry && registry->all_of<RectTransform>((entt::entity)entityID))
        registry->get<RectTransform>((entt::entity)entityID).anchor = (UIAnchor)anchor;
}

bool ScriptGlue::RectTransform_IsActive(uint32_t entityID)
{
    auto *registry = ScriptManager::GetActiveRegistry();
    if (registry && registry->all_of<RectTransform>((entt::entity)entityID))
        return registry->get<RectTransform>((entt::entity)entityID).active;
    return false;
}

void ScriptGlue::RectTransform_SetActive(uint32_t entityID, bool active)
{
    auto *registry = ScriptManager::GetActiveRegistry();
    if (registry && registry->all_of<RectTransform>((entt::entity)entityID))
        registry->get<RectTransform>((entt::entity)entityID).active = active;
}

// --- UI Components ---
bool ScriptGlue::UIButton_IsClicked(uint32_t entityID)
{
    auto *registry = ScriptManager::GetActiveRegistry();
    if (registry && registry->all_of<UIButton>((entt::entity)entityID))
        return registry->get<UIButton>((entt::entity)entityID)
            .isPressed; // Assuming isPressed means clicked in this frame context
    return false;
}

float ScriptGlue::UIText_GetFontSize(uint32_t entityID)
{
    auto *registry = ScriptManager::GetActiveRegistry();
    if (registry && registry->all_of<UIText>((entt::entity)entityID))
        return registry->get<UIText>((entt::entity)entityID).fontSize;
    return 0.0f;
}

void ScriptGlue::UIText_SetFontSize(uint32_t entityID, float size)
{
    auto *registry = ScriptManager::GetActiveRegistry();
    if (registry && registry->all_of<UIText>((entt::entity)entityID))
        registry->get<UIText>((entt::entity)entityID).fontSize = size;
}

// --- ImGui ---
void ScriptGlue::ImGui_Text(void *text)
{
    ImGui::Text("%ls", (const wchar_t *)text);
}
bool ScriptGlue::ImGui_Button(void *label)
{
    return ImGui::Button((const char *)label);
} // Note: Need to handle UTF16/UTF8 properly
bool ScriptGlue::ImGui_Checkbox(void *label, bool *value)
{
    return ImGui::Checkbox((const char *)label, value);
}
bool ScriptGlue::ImGui_SliderFloat(void *label, float *value, float min, float max)
{
    return ImGui::SliderFloat((const char *)label, value, min, max);
}
void ScriptGlue::ImGui_Begin(void *name)
{
    ImGui::Begin((const char *)name);
}
void ScriptGlue::ImGui_End()
{
    ImGui::End();
}
void ScriptGlue::ImGui_SameLine()
{
    ImGui::SameLine();
}
void ScriptGlue::ImGui_Separator()
{
    ImGui::Separator();
}

bool ScriptGlue::Entity_HasComponent(uint32_t entityID, void *componentName)
{
    // Simplified stub
    return true;
}

// String helpers (Stubs for now)
void *ScriptGlue::UIButton_GetText(uint32_t entityID)
{
    return (void *)L"Button";
}
void ScriptGlue::UIButton_SetText(uint32_t entityID, void *text)
{
}
void *ScriptGlue::UIText_GetText(uint32_t entityID)
{
    return (void *)L"Text";
}
void ScriptGlue::UIText_SetText(uint32_t entityID, void *text)
{
}

void ScriptGlue::RegisterFunctions()
{
    std::wstring assemblyPath = L"bin/managed/ChainedEngine.Managed.dll";

    typedef void (*InitFn)(void *, void *, void *, void *, void *, void *, void *, void *, void *);
    auto initFunc = (InitFn)DotNetHost::GetManagedFunction(
        assemblyPath, L"ChainedEngine.InternalCalls, ChainedEngine.Managed", L"Initialize");

    if (initFunc)
    {
        initFunc((void *)Log_Info, (void *)Log_Warning, (void *)Log_Error,
                 (void *)Transform_GetPosition, (void *)Transform_SetPosition,
                 (void *)RectTransform_GetPosition, (void *)RectTransform_SetPosition,
                 (void *)RectTransform_GetSize, (void *)RectTransform_SetSize);
    }

    // Retrieve C# Lifecycle Hooks
    s_Delegates.CreateInstance = (CreateInstanceFn)DotNetHost::GetManagedFunction(
        assemblyPath, L"ChainedEngine.Entity, ChainedEngine.Managed", L"CreateInstance");
    s_Delegates.OnCreate = (OnCreateFn)DotNetHost::GetManagedFunction(
        assemblyPath, L"ChainedEngine.Entity, ChainedEngine.Managed", L"CallOnCreate");
    s_Delegates.OnUpdate = (OnUpdateFn)DotNetHost::GetManagedFunction(
        assemblyPath, L"ChainedEngine.Entity, ChainedEngine.Managed", L"CallOnUpdate");
    s_Delegates.DestroyInstance = (DestroyInstanceFn)DotNetHost::GetManagedFunction(
        assemblyPath, L"ChainedEngine.Entity, ChainedEngine.Managed", L"DestroyInstance");
}
} // namespace CHEngine
