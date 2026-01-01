#ifndef SCRIPT_GLUE_H
#define SCRIPT_GLUE_H

#include <string>

namespace CHEngine
{
class ScriptGlue
{
public:
    static void RegisterFunctions();

    // C# lifecycle delegates
    // Internal function pointers retrieved from C#
    typedef void *(*CreateInstanceFn)(uint32_t, void *);
    typedef void (*OnCreateFn)(void *);
    typedef void (*OnUpdateFn)(void *, float);
    typedef void (*DestroyInstanceFn)(void *);

    struct ScriptDelegates
    {
        CreateInstanceFn CreateInstance;
        OnCreateFn OnCreate;
        OnUpdateFn OnUpdate;
        DestroyInstanceFn DestroyInstance;
    };

    static ScriptDelegates &GetDelegates();

    // --- Internal Calls (Bindings) ---
    static void Log_Info(void *message);
    static void Log_Warning(void *message);
    static void Log_Error(void *message);

    // Transform
    static void Transform_GetPosition(uint32_t entityID, void *outPos);
    static void Transform_SetPosition(uint32_t entityID, void *inPos);

    // RectTransform
    static void RectTransform_GetPosition(uint32_t entityID, void *outPos);
    static void RectTransform_SetPosition(uint32_t entityID, void *inPos);
    static void RectTransform_GetSize(uint32_t entityID, void *outSize);
    static void RectTransform_SetSize(uint32_t entityID, void *inSize);
    static uint8_t RectTransform_GetAnchor(uint32_t entityID);
    static void RectTransform_SetAnchor(uint32_t entityID, uint8_t anchor);
    static bool RectTransform_IsActive(uint32_t entityID);
    static void RectTransform_SetActive(uint32_t entityID, bool active);

    // UI Components
    static void *UIButton_GetText(uint32_t entityID);
    static void UIButton_SetText(uint32_t entityID, void *text);
    static bool UIButton_IsClicked(uint32_t entityID);
    static void *UIText_GetText(uint32_t entityID);
    static void UIText_SetText(uint32_t entityID, void *text);
    static float UIText_GetFontSize(uint32_t entityID);
    static void UIText_SetFontSize(uint32_t entityID, float size);

    // Entity
    static bool Entity_HasComponent(uint32_t entityID, void *componentName);

    // ImGui
    static void ImGui_Text(void *text);
    static bool ImGui_Button(void *label);
    static bool ImGui_Checkbox(void *label, bool *value);
    static bool ImGui_SliderFloat(void *label, float *value, float min, float max);
    static void ImGui_Begin(void *name);
    static void ImGui_End();
    static void ImGui_SameLine();
    static void ImGui_Separator();
};
} // namespace CHEngine

#endif // SCRIPT_GLUE_H
