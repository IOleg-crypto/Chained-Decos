#ifndef CH_UI_RENDER_HELPERS_H
#define CH_UI_RENDER_HELPERS_H

#include "engine/scene/components/control_component.h"
#include "engine/assets/types/texture_asset.h"
#include "engine/assets/asset_manager.h"
#include "engine/core/service_locator.h"
#include "imgui.h"

namespace Chained
{

inline ImVec4 ToImVec4(const Color& c)
{
    return ImVec4(c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, c.a / 255.0f);
}

inline ImU32 ToImU32(const Color& c)
{
    return IM_COL32(c.r, c.g, c.b, c.a);
}

inline ImU32 GetControlColor(const UIStyle& style, const UIControlComponent& wc)
{
    if (wc.IsDown)    return ToImU32(style.PressedColor);
    if (wc.IsHovered) return ToImU32(style.HoverColor);
    return ToImU32(style.BackgroundColor);
}

inline void RenderAlignedTextureText(ImDrawList* dl, ImFont* font, float fontSize, const std::string& text,
                                     const ImVec2& pos, const ImVec2& size, const TextStyle& textStyle)
{
    if (text.empty()) return;

    ImVec2 textSize;
    if (font)
    {
        ImGui::PushFont(font);
        textSize = ImGui::CalcTextSize(text.c_str());
        ImGui::PopFont();
    }
    else
    {
        textSize = ImGui::CalcTextSize(text.c_str());
    }

    ImVec2 textPos = pos;

    if (textStyle.Horizontal == HorizontalAlignment::Center)
        textPos.x += (size.x - textSize.x) * 0.5f;
    else if (textStyle.Horizontal == HorizontalAlignment::Right)
        textPos.x += (size.x - textSize.x);

    if (textStyle.Vertical == VerticalAlignment::Center)
        textPos.y += (size.y - textSize.y) * 0.5f;
    else if (textStyle.Vertical == VerticalAlignment::Bottom)
        textPos.y += (size.y - textSize.y);

    if (textStyle.Shadow)
    {
        ImVec2 shadowPos = { textPos.x + textStyle.ShadowOffset, textPos.y + textStyle.ShadowOffset };
        if (font) dl->AddText(font, fontSize, shadowPos, ToImU32(textStyle.ShadowColor), text.c_str());
        else      dl->AddText(shadowPos, ToImU32(textStyle.ShadowColor), text.c_str());
    }

    if (font) dl->AddText(font, fontSize, textPos, ToImU32(textStyle.TextColor), text.c_str());
    else      dl->AddText(textPos, ToImU32(textStyle.TextColor), text.c_str());
}

inline std::shared_ptr<TextureAsset> ResolveTexture(AssetHandle& handle, const std::string& path)
{
    auto* am = ServiceLocator::Get<AssetManager>();
    if (!am) return nullptr;

    if (handle == 0 && !path.empty())
    {
        auto asset = am->Get<TextureAsset>(path);
        if (asset)
            handle = static_cast<AssetHandle>(asset->GetID());
        return asset;
    }

    if (handle != 0)
        return am->Get<TextureAsset>(handle);

    return nullptr;
}

} // namespace Chained

#endif // CH_UI_RENDER_HELPERS_H
