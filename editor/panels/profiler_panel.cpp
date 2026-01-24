#include "profiler_panel.h"
#include "engine/core/profiler.h"
#include <format>
#include <imgui.h>

namespace CHEngine
{
ProfilerPanel::ProfilerPanel()
{
    m_Name = "Profiler";
}

void ProfilerPanel::OnImGuiRender(bool readOnly)
{
    if (!m_IsOpen)
        return;

    ImGui::Begin(m_Name.c_str(), &m_IsOpen);

    const auto &stats = Profiler::GetStats();

    if (ImGui::CollapsingHeader("Hardware & System", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Text("GPU: %s", stats.GPU.c_str());
        ImGui::Text("CPU: %s", stats.CPU.c_str());
        ImGui::Separator();
        float usedGB = stats.UsedRAM / (1024.0f * 1024.0f * 1024.0f);
        float totalGB = stats.TotalRAM / (1024.0f * 1024.0f * 1024.0f);
        if (totalGB > 0)
            ImGui::Text("RAM: %.2f GB / %.2f GB", usedGB, totalGB);
        else
            ImGui::Text("RAM: %.2f GB used", usedGB);
    }

    if (ImGui::CollapsingHeader("Scene Statistics", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Columns(2);
        ImGui::Text("Entities:");
        ImGui::NextColumn();
        ImGui::Text("%u", stats.EntityCount);
        ImGui::NextColumn();
        ImGui::Text("Draw Calls:");
        ImGui::NextColumn();
        ImGui::Text("%u", stats.DrawCalls);
        ImGui::NextColumn();
        ImGui::Text("Meshes:");
        ImGui::NextColumn();
        ImGui::Text("%u", stats.MeshCount);
        ImGui::NextColumn();

        // Format polys (K, M)
        std::string polyStr;
        if (stats.PolyCount > 1000000)
            polyStr = std::format("{:.2f} M", stats.PolyCount / 1000000.0f);
        else if (stats.PolyCount > 1000)
            polyStr = std::format("{:.1f} K", stats.PolyCount / 1000.0f);
        else
            polyStr = std::to_string(stats.PolyCount);

        ImGui::Text("Polygons:");
        ImGui::NextColumn();
        ImGui::Text("%s", polyStr.c_str());
        ImGui::NextColumn();
        ImGui::Text("Colliders:");
        ImGui::NextColumn();
        ImGui::Text("%u", stats.ColliderCount);
        ImGui::NextColumn();
        ImGui::Columns(1);
    }

    const auto &results = Profiler::GetLastFrameResults();
    if (ImGui::CollapsingHeader("Execution Timeline", ImGuiTreeNodeFlags_DefaultOpen))
    {
        const auto &history = Profiler::GetFrameTimeHistory();
        if (!history.empty())
        {
            float max = 0.0f;
            for (float f : history)
                if (f > max)
                    max = f;

            ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(0.2f, 0.7f, 1.0f, 1.0f));
            ImGui::PlotLines("##FrameTime", history.data(), (int)history.size(), 0,
                             std::format("Max: {:.2f}ms", max).c_str(), 0.0f, 33.3f, ImVec2(0, 80));
            ImGui::PopStyleColor();
        }

        for (const auto &result : results)
        {
            DrawProfileResult(*result);
        }
    }

    ImGui::End();
}

void ProfilerPanel::DrawProfileResult(const ProfileResult &result)
{
    float durationMs = result.Duration.count() / 1000.0f;
    std::string label = std::format("{} - {:.3f}ms", result.Name, durationMs);

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth;
    if (result.Children.empty())
        flags |= ImGuiTreeNodeFlags_Leaf;

    bool opened = ImGui::TreeNodeEx(label.c_str(), flags);

    if (opened)
    {
        for (const auto &child : result.Children)
        {
            DrawProfileResult(*child);
        }
        ImGui::TreePop();
    }
}

} // namespace CHEngine
