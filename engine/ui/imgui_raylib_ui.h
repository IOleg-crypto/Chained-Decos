#ifndef CH_IMGUI_RAYLIB_UI_H
#define CH_IMGUI_RAYLIB_UI_H

#include <imgui.h>
#include <raylib.h>

namespace CHEngine::UI
{

inline void DrawImage(const Texture2D &texture, float width, float height)
{
    ImGui::Image((ImTextureID)(uintptr_t)texture.id, ImVec2(width, height));
}

inline void DrawRenderTexture(const RenderTexture2D &texture, float width, float height)
{
    // RenderTexture is flipped vertically in OpenGL
    ImGui::Image((ImTextureID)(uintptr_t)texture.texture.id, ImVec2(width, height), ImVec2(0, 1),
                 ImVec2(1, 0));
}

inline void DrawImageFit(const Texture2D &texture, float width, float height, bool center = true)
{
    float aspect = (float)texture.width / (float)texture.height;
    float targetAspect = width / height;

    float w = width;
    float h = height;

    if (aspect > targetAspect)
        h = width / aspect;
    else
        w = height * aspect;

    if (center)
    {
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (width - w) * 0.5f);
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (height - h) * 0.5f);
    }

    DrawImage(texture, w, h);
}

} // namespace CHEngine::UI

// Compatibility macros to avoid changing too much code at once
#define rlImGuiImageSize(tex, w, h) CHEngine::UI::DrawImage(*(tex), (float)(w), (float)(h))
#define rlImGuiImageRenderTextureFit(tex, center)                                                  \
    CHEngine::UI::DrawRenderTexture(*(tex), ImGui::GetContentRegionAvail().x,                      \
                                    ImGui::GetContentRegionAvail().y)

#endif // CH_IMGUI_RAYLIB_UI_H
