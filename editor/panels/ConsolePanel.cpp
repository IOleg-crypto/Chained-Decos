#include "ConsolePanel.h"
#include <imgui.h>

namespace CHEngine
{
ConsolePanel::ConsolePanel()
{
    Log("Console initialized");
}

void ConsolePanel::OnImGuiRender()
{
    ImGui::Begin("Console");

    if (ImGui::Button("Clear"))
        Clear();

    ImGui::Separator();

    ImGui::BeginChild("ScrollingRegion", ImVec2(0, 0), ImGuiChildFlags_None,
                      ImGuiWindowFlags_HorizontalScrollbar);

    for (auto &msg : m_Messages)
    {
        ImVec4 color = ImVec4(1, 1, 1, 1);
        if (msg.level == LogMessage::Level::Warn)
            color = ImVec4(1, 1, 0, 1);
        if (msg.level == LogMessage::Level::Error)
            color = ImVec4(1, 0, 0, 1);

        ImGui::PushStyleColor(ImGuiCol_Text, color);
        ImGui::TextUnformatted(msg.message.c_str());
        ImGui::PopStyleColor();
    }

    if (m_AutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
        ImGui::SetScrollHereY(1.0f);

    ImGui::EndChild();

    ImGui::End();
}

void ConsolePanel::Log(const std::string &message, LogMessage::Level level)
{
    m_Messages.push_back({level, message, ""});
    if (m_Messages.size() > MAX_MESSAGES)
        m_Messages.pop_front();
}

void ConsolePanel::Clear()
{
    m_Messages.clear();
}
} // namespace CHEngine
