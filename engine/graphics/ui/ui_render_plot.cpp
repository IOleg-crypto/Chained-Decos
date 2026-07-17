// ui_render_plot.cpp
// Renders: Plot (lines + histogram)
#include "ui_render_helpers.h"

namespace Chained
{

bool RenderPlot(const PlotData& plot, UIControlComponent& wc, const ImVec2& pos, const ImVec2& size)
{
    ImGui::SetCursorScreenPos(pos);
    const char* overlay = plot.OverlayText.empty() ? nullptr : plot.OverlayText.c_str();
    if (plot.Mode == PlotMode::Lines)
    {
        ImGui::PlotLines("##plotlines", plot.Values.data(), static_cast<int>(plot.Values.size()),
                         0, overlay, plot.ScaleMin, plot.ScaleMax, {size.x, size.y});
    }
    else
    {
        ImGui::PlotHistogram("##plothisto", plot.Values.data(), static_cast<int>(plot.Values.size()),
                             0, overlay, plot.ScaleMin, plot.ScaleMax, {size.x, size.y});
    }
    return false;
}

} // namespace Chained
