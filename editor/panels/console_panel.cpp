#include "console_panel.h"
#include <imgui.h>

namespace CHEngine
{
ConsolePanel *ConsolePanel::s_Instance = nullptr;

ConsolePanel::ConsolePanel()
{
    s_Instance = this;
}

void ConsolePanel::AddLog(const char *message, ConsoleLogLevel level)
{
    if (s_Instance)
        s_Instance->Log(message, level);
}

void ConsolePanel::OnImGuiRender(bool readOnly)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 10));
    ImGui::Begin("Console");

    ImGui::BeginDisabled(readOnly);
    if (ImGui::Button("Clear"))
        Clear();
    ImGui::EndDisabled();

    ImGui::Separator();
    ImGui::Spacing();

    ImGui::BeginChild("ScrollingRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
    ImGui::PopStyleVar();

    for (const auto &msg : m_Messages)
    {
        ImVec4 color = ImVec4(1, 1, 1, 1);
        if (msg.level == ConsoleLogLevel::Warn)
            color = ImVec4(1, 1, 0, 1);
        if (msg.level == ConsoleLogLevel::Error)
            color = ImVec4(1, 0, 0, 1);

        ImGui::PushStyleColor(ImGuiCol_Text, color);
        ImGui::TextUnformatted(msg.message.c_str());
        ImGui::PopStyleColor();
    }

    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
        ImGui::SetScrollHereY(1.0f);

    ImGui::EndChild();
    ImGui::End();
}

void ConsolePanel::Log(const std::string &message, ConsoleLogLevel level)
{
    ConsoleLogEntry entry;
    entry.level = level;
    entry.message = message;
    m_Messages.push_back(entry);

    if (m_Messages.size() > MAX_MESSAGES)
        m_Messages.pop_front();
}

void ConsolePanel::Clear()
{
    m_Messages.clear();
}
} // namespace CHEngine
