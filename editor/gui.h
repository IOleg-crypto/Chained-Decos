#ifndef CH_EDITOR_GUI_H
#define CH_EDITOR_GUI_H

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "layer.h"
#include "panels.h"
#include "engine/foundation/color.h"
#include "engine/graphics/pipeline/renderer.h"

namespace Chained
{
// Immediate-mode GUI helpers shared by editor panels and property inspectors.
class EditorGUI
{
public:
    // Draws the main editor menu bar for the provided panel set.
    static void DrawMenuBar(EditorLayer& editorLayer, EditorPanels& panels);

    // Property layout helpers.
    static void BeginPropertyGrid();
    static void EndPropertyGrid();
    static void BeginProperty(const char* label);
    static void EndProperty();

    // Simple property widgets that do not use columns.
    static bool Property(const char* label, bool& value);
    static bool Property(const char* label, int& value, int min = 0, int max = 0);
    static bool Property(const char* label, float& value, float speed = 0.1f, float min = 0.0f, float max = 0.0f);
    static bool Property(const char* label, std::string& value, bool multiline = false);
    static bool Property(const char* label, Color& value);
    static bool Property(const char* label, glm::vec2& value, float speed = 0.1f, float min = 0.0f, float max = 0.0f);
    static bool Property(const char* label, glm::vec3& value, float speed = 0.1f, float min = 0.0f, float max = 0.0f);
    static bool Property(const char* label, glm::vec4& value, float speed = 0.1f, float min = 0.0f, float max = 0.0f);
    static bool Property(const char* label, uint64_t& value);

    static bool Property(const char* label, int& value, const char** items, int itemCount);

    // Action widgets.
    static bool ActionButton(const char* icon, const char* label);

    // File property widgets.
    static bool FileProperty(const char* label, std::string& value, const char* filter = nullptr);
    static bool FileProperty(const char* label, std::string& path, uint32_t textureId, const char* filter = nullptr);

    static bool DrawVec2(const char* label, glm::vec2& values, float resetValue = 0.0f);
    static bool DrawVec3(const char* label, glm::vec3& values, float resetValue = 0.0f);
    static bool DrawVec4(const char* label, glm::vec4& values, float resetValue = 0.0f);
    // Applies the editor-wide ImGui style.
    static void ApplyTheme();
};

} // namespace Chained

#endif // CH_EDITOR_GUI_H
