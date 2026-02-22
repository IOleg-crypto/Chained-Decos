#ifndef CH_IMGUI_CONVERTER_H
#define CH_IMGUI_CONVERTER_H

#include "imgui.h"
#include "raylib.h"

namespace CHEngine
{
class ImGuiConverter
{
public:
    static ImVec4 ToImVec4(Color color)
    {
        return {color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f};
    }

    static ImVec2 ToImVec2(Vector2 vec)
    {
        return {vec.x, vec.y};
    }

    static Vector2 ToVector2(ImVec2 vec)
    {
        return {vec.x, vec.y};
    }

    static ImTextureID ToImTextureID(unsigned int textureID)
    {
        return (ImTextureID)(intptr_t)textureID;
    }

    static void ToFloat4(Color color, float* out)
    {
        out[0] = color.r / 255.0f;
        out[1] = color.g / 255.0f;
        out[2] = color.b / 255.0f;
        out[3] = color.a / 255.0f;
    }

    static Color FromFloat4(const float* color)
    {
        return {(unsigned char)(color[0] * 255.0f), (unsigned char)(color[1] * 255.0f),
                (unsigned char)(color[2] * 255.0f), (unsigned char)(color[3] * 255.0f)};
    }
};
} // namespace CHEngine

#endif // CH_IMGUI_CONVERTER_H
