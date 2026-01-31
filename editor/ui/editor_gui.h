#ifndef CH_EDITOR_GUI_H
#define CH_EDITOR_GUI_H

#include "editor/editor_types.h"
#include "engine/scene/scene.h"
#include "raylib.h"
#include <string>
#include <memory> // Added for std::shared_ptr
#include <vector> // Added for std::vector
#include <glm/glm.hpp>

namespace CHEngine
{
class EditorLayer;
class Panel; // Added forward declaration for Panel
struct RectTransform; // Forward declaration
} // namespace CHEngine

namespace CHEngine::EditorUI
{

struct MenuBarState
{
    bool IsPlaying = false;
    std::vector<std::shared_ptr<Panel>> *Panels = nullptr;
};

class GUI
{
public:
    static void DrawMenuBar(const MenuBarState &state, const EventCallbackFn &callback);
    static void DrawToolbar(bool isPlaying, const EventCallbackFn &callback);
    static void DrawProjectSelector(bool active, Texture2D icon, std::function<void()> onNew,
                                    std::function<void()> onOpen, std::function<void()> onExit);

    static void BeginProperties(float columnWidth = 100.0f);
    static void EndProperties();

    static bool DrawVec3Control(const std::string &label, Vector3 &values, float resetValue = 0.0f,
                                float columnWidth = 100.0f);
    static bool DrawVec2Control(const std::string &label, glm::vec2 &values, float resetValue = 0.0f,
                                float columnWidth = 100.0f);
    static bool Property(const char *label, bool &value);
    static bool Property(const char *label, int &value);
    static bool Property(const char *label, float &value, float speed = 0.1f, float min = 0.0f,
                         float max = 0.0f);
    static bool Property(const char *label, std::string &value, bool multiline = false);
    static bool Property(const char *label, Color &value);
    static bool Property(const char *label, Vector2 &value, float speed = 0.1f, float min = 0.0f,
                         float max = 0.0f);
    static bool Property(const char *label, Vector3 &value, float speed = 0.1f, float min = 0.0f,
                         float max = 0.0f);
    
    // GLM Overloads
    static bool Property(const char *label, glm::vec2 &value, float speed = 0.1f, float min = 0.0f,
                         float max = 0.0f);
    static bool Property(const char *label, glm::vec3 &value, float speed = 0.1f, float min = 0.0f,
                         float max = 0.0f);

    static bool Property(const char *label, int &value, const char **items, int itemCount);

    static Camera3D GetActiveCamera(SceneState state);
    static Ray GetMouseRay(const Camera3D &camera, Vector2 viewportPos, Vector2 viewportSize);
    static void CalculatePlayCamera(Camera3D &camera, Scene *scene);


    static void SetDarkThemeColors();
};
} // namespace CHEngine::EditorUI

#endif // CH_EDITOR_GUI_H
