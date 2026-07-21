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
    return ToImU32(style.State.CurrentColor);
}

inline void RenderAlignedTextureText(ImDrawList* dl, ImFont* font, float fontSize, const std::string& text,
                                     const ImVec2& pos, const ImVec2& size, const TextStyle& textStyle)
{
    if (text.empty()) return;

    // No per-widget font -> fall back to the current (default) font, but still
    // honor the widget's FontSize. ImGui 1.92 renders any font at any size.
    ImFont* activeFont = font ? font : ImGui::GetFont();
    const float activeSize = (fontSize > 0.0f) ? fontSize : ImGui::GetFontSize();

    // Measure at the exact size the text will be drawn at.
    ImGui::PushFont(activeFont, activeSize);
    ImVec2 textSize = ImGui::CalcTextSize(text.c_str());
    ImGui::PopFont();

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
        dl->AddText(activeFont, activeSize, shadowPos, ToImU32(textStyle.ShadowColor), text.c_str());
    }

    dl->AddText(activeFont, activeSize, textPos, ToImU32(textStyle.TextColor), text.c_str());
}

inline std::shared_ptr<TextureAsset> ResolveTexture(AssetHandle& handle, const std::string& path)
{
    auto* am = ServiceLocator::Get<AssetManager>();
    if (!am) return nullptr;

    if (!path.empty())
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
