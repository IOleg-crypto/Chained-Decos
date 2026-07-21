// ui_render_image.cpp
// Renders: Image, ImageButton
#include "ui_render_helpers.h"

namespace Chained
{

bool RenderImage(ImageData& img, UIControlComponent& wc, const ImVec2& pos, const ImVec2& size)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 pMax = {pos.x + size.x, pos.y + size.y};

    bool changed = false;
    auto textureAsset = ResolveTexture(img.TextureHandle, img.TexturePath);
    if (textureAsset && textureAsset->GetState() == AssetState::Ready)
    {
        if (auto tex = textureAsset->GetTexture())
        {
            ImTextureID texId = (ImTextureID)(uintptr_t)tex->GetNativeHandle();
            dl->AddImageRounded(texId, pos, pMax, {0,0}, {1,1}, ToImU32(img.TintColor), wc.BoxStyle.Rounding);
            changed = true;
        }
    }
    else if (!img.TexturePath.empty())
    {
        dl->AddRectFilled(pos, pMax, IM_COL32(60, 60, 60, 200), wc.BoxStyle.Rounding);
        changed = true;
    }
    else
    {
        dl->AddRectFilled(pos, pMax, ToImU32(wc.BoxStyle.BackgroundColor), wc.BoxStyle.Rounding);
        changed = true;
    }

    if (wc.BoxStyle.BorderSize > 0.0f)
        dl->AddRect(pos, pMax, ToImU32(wc.BoxStyle.BorderColor), wc.BoxStyle.Rounding, 0, wc.BoxStyle.BorderSize);

    return changed;
}

bool RenderImageButton(ImageButtonData& imgBtn, UIControlComponent& wc, const ImVec2& pos, const ImVec2& size, ImFont* font, const TextStyle& textStyle)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 pMax = {pos.x + size.x, pos.y + size.y};

    ImU32 btnColor = GetControlColor(wc.BoxStyle, wc);
    dl->AddRectFilled(pos, pMax, btnColor, wc.BoxStyle.Rounding);

    float pad = imgBtn.FramePadding >= 0 ? static_cast<float>(imgBtn.FramePadding) : 4.0f;
    ImVec2 imgPos = { pos.x + pad, pos.y + pad };
    ImVec2 imgMax = { pMax.x - pad, pMax.y - pad };

    auto textureAsset = ResolveTexture(imgBtn.TextureHandle, imgBtn.TexturePath);
    if (textureAsset && textureAsset->GetState() == AssetState::Ready)
    {
        if (auto tex = textureAsset->GetTexture())
        {
            ImTextureID texId = (ImTextureID)(uintptr_t)tex->GetNativeHandle();
            dl->AddImageRounded(texId, imgPos, imgMax, {0,0}, {1,1}, ToImU32(imgBtn.TintColor), wc.BoxStyle.Rounding);
        }
    }

    if (!imgBtn.Label.empty())
        RenderAlignedTextureText(dl, font, textStyle.FontSize, imgBtn.Label, pos, size, textStyle);

    if (wc.BoxStyle.BorderSize > 0.0f)
        dl->AddRect(pos, pMax, ToImU32(wc.BoxStyle.BorderColor), wc.BoxStyle.Rounding, 0, wc.BoxStyle.BorderSize);

    return wc.PressedThisFrame;
}

} // namespace Chained
