#ifndef CH_UI_RENDERER_H
#define CH_UI_RENDERER_H

#include "engine/scene/scene.h"
#include "engine/scene/components/control_component.h"
#include "imgui.h"

namespace CHEngine
{
    struct UIRendererData
    {
        // Internal state like input buffers could go here
        std::map<entt::entity, std::vector<char>> InputBuffers;
    };

    class UIRenderer
    {
    public:
        static void Init();
        static void Shutdown();

        UIRenderer();
        ~UIRenderer();

        // Main entry point for drawing UI for a scene.
        void DrawCanvas(Scene* scene, const ImVec2& referencePosition, const ImVec2& referenceSize, bool editMode = false);

        // Helper to calculate the absolute screen-space rect for a UI entity, respecting hierarchy.
        Rectangle GetEntityRect(Entity entity, const ImVec2& viewportSize, const ImVec2& viewportOffset);

        static UIRenderer& Get() 
        { 
            CH_CORE_ASSERT(s_Instance, "UIRenderer instance is null!");
            return *s_Instance; 
        }

        inline UIRendererData& GetData() { return *m_Data; }
        inline const UIRendererData& GetData() const { return *m_Data; }

    public:
        // Helper scope for UI styling
        struct UIStyleScope
        {
            int ColorPushCount = 0;
            int VarPushCount = 0;
            int FontPushCount = 0;
            float OldFontScale = 1.0f;
            bool Disabled = false;

            UIStyleScope() {}
            ~UIStyleScope();

            void PushStyle(const UIStyle& style, bool interactable = true);
            void PushText(const TextStyle& text);
            void PushFont(const std::string& fontName, float fontSize = 0.0f);
        };

    private:
        static UIRenderer* s_Instance;
        std::unique_ptr<UIRendererData> m_Data;
    };
}

#endif // CH_UI_RENDERER_H
