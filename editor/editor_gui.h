#ifndef CH_EDITOR_GUI_H
#define CH_EDITOR_GUI_H

#include <functional>
#include <string>
#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include <raylib.h>
#include "editor_layer.h"
#include "editor_panels.h"

namespace CHEngine
{
    class EditorGUI
    {
    public:
        // --- Menu System ---
        static void DrawMenuBar(EditorPanels& panels);

        // --- Property Widgets (Single) ---
        // Simple declarative widgets that don't use columns
        static bool Property(const char *label, bool &value);
        static bool Property(const char *label, int &value, int min = 0, int max = 0);
        static bool Property(const char *label, float &value, float speed = 0.1f, float min = 0.0f, float max = 0.0f);
        static bool Property(const char *label, std::string &value, bool multiline = false);
        static bool Property(const char *label, Color &value);
        static bool Property(const char *label, Vector2 &value, float speed = 0.1f, float min = 0.0f, float max = 0.0f);
        static bool Property(const char *label, Vector3 &value, float speed = 0.1f, float min = 0.0f, float max = 0.0f);
        static bool Property(const char *label, glm::vec2 &value, float speed = 0.1f, float min = 0.0f, float max = 0.0f);
        static bool Property(const char *label, glm::vec3 &value, float speed = 0.1f, float min = 0.0f, float max = 0.0f);
        static bool Property(const char *label, int &value, const char **items, int itemCount);

        // Builder for properties
        struct PropertyBuilder
        {
            bool Changed = false;

            PropertyBuilder& Bool(const char* label, bool& value) { Changed |= Property(label, value); return *this; }
            PropertyBuilder& Int(const char* label, int& value, int min = 0, int max = 0) { Changed |= Property(label, value, min, max); return *this; }
            PropertyBuilder& Float(const char* label, float& value, float speed = 0.1f, float min = 0.0f, float max = 0.0f) { Changed |= Property(label, value, speed, min, max); return *this; }
            PropertyBuilder& String(const char* label, std::string& value, bool multiline = false) { Changed |= Property(label, value, multiline); return *this; }
            PropertyBuilder& Color(const char* label, ::Color& value) { Changed |= Property(label, value); return *this; }
            PropertyBuilder& Vec2(const char* label, Vector2& value, float speed = 0.1f) { Changed |= Property(label, value, speed); return *this; }
            PropertyBuilder& Vec3(const char* label, Vector3& value, float speed = 0.1f) { Changed |= Property(label, value, speed); return *this; }
            PropertyBuilder& Vec2(const char* label, glm::vec2& value, float speed = 0.1f) { Changed |= Property(label, value, speed); return *this; }
            PropertyBuilder& Vec3(const char* label, glm::vec3& value, float speed = 0.1f) { Changed |= Property(label, value, speed); return *this; }
            
            operator bool() const { return Changed; }
        };

        static PropertyBuilder Begin() { return PropertyBuilder{}; }

    public:
        // --- Utils ---
        static void ApplyTheme();
        static Camera3D GetActiveCamera(SceneState state);
        static Ray GetMouseRay(const Camera3D &camera, Vector2 localMousePos, Vector2 viewportSize);
        
        static bool DrawVec3(const std::string &label, Vector3 &values, float resetValue = 0.0f);
        static bool DrawVec3(const std::string &label, glm::vec3 &values, float resetValue = 0.0f);
        static bool DrawVec2(const std::string &label, Vector2 &values, float resetValue = 0.0f);
        static bool DrawVec2(const std::string &label, glm::vec2 &values, float resetValue = 0.0f);

        // --- Action Widgets ---
        static bool ActionButton(const char* icon, const char* label);
    };

} // namespace CHEngine

#endif // CH_EDITOR_GUI_H
