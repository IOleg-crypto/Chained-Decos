#ifndef CH_RENDER_H
#define CH_RENDER_H

#include "engine/core/base.h"
#include "engine/core/timestep.h"
#include "engine/graphics/render_types.h"
#include "engine/graphics/environment.h"
#include "engine/scene/scene.h"
#include "raylib.h"
#include <string>
#include <vector>

namespace CHEngine
{
    /**
     * The primary interface for all rendering operations in the engine.
     * Consolidates scene rendering, primitive drawing, and low-level API management.
     * Following the 'Action-Based' naming convention (Render instead of Renderer).
     */
    class Render
    {
    public: // Life Cycle
        /** Initializes the rendering engine and underlying hardware context. */
        static void Init();

        /** Shuts down the rendering engine and releases all GPU resources. */
        static void Shutdown();

    public: // Scene Rendering (Hazel-style)
        /** Prepares the renderer for a new scene using the specified camera. */
        static void BeginScene(const Camera3D& camera);

        /** Finalizes the scene rendering and submits all commands to the GPU. */
        static void EndScene();

        /** High-level method to render an entire scene with optional debug flags. */
        static void DrawScene(Scene* scene, const Camera3D& camera, Timestep ts = 0, const DebugRenderFlags* debugFlags = nullptr);

    public: // Direct Draw Commands
        /** Clears the screen buffer with the specified color. */
        static void Clear(Color color = BLACK);

        /** Sets the current rendering viewport. */
        static void SetViewport(int x, int y, int width, int height);

        /** Draws a model at a specific transform with optional material overrides. */
        static void DrawModel(const std::string& path, const Matrix& transform, 
                             const std::vector<MaterialSlot>& overrides = {}, 
                             int animIndex = -1, int frame = 0);

        /** Draws a lines in 3D space. */
        static void DrawLine(Vector3 start, Vector3 end, Color color);

        /** Draws a 3D grid for editor visualization. */
        static void DrawGrid(int slices, float spacing);

        /** Renders a skybox using the provided settings. */
        static void DrawSkybox(const SkyboxSettings& skybox, const Camera3D& camera);

    public: // UI Rendering (Consolidated)
        /** Draws UI elements for the scene. */
        static void DrawUI(Scene* scene, const ImVec2& refPos = {0, 0}, const ImVec2& refSize = {0, 0}, bool editMode = false);

    public: // State Management
        /** Returns the current internal state of the renderer. */
        static RendererState& GetState();

        /** Configures the primary directional light source. */
        static void SetDirectionalLight(Vector3 direction, Color color);

        /** Sets the ambient light intensity for the scene. */
        static void SetAmbientLight(float intensity);

        /** Applies environment-wide settings (fog, lighting, skybox). */
        static void ApplyEnvironment(const EnvironmentSettings& settings);

    private: // Internal Helpers
        static void RenderModels(Scene* scene, Timestep ts);
        static void RenderDebug(Scene* scene, const DebugRenderFlags* debugFlags);
        static void RenderEditorIcons(Scene* scene, const Camera3D& camera);
        static void InitSkybox();

    private: // Internal State
        static RendererState s_State;
    };
}

#endif // CH_RENDER_H
