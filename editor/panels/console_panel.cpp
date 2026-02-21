#include "console_panel.h"
#include "imgui.h"
#include "raylib.h"

namespace CHEngine
{
ConsolePanel* ConsolePanel::s_Instance = nullptr;

ConsolePanel::ConsolePanel()
{
    s_Instance = this;
    m_Name = "Console";
}

ConsolePanel::~ConsolePanel()
{
    if (s_Instance == this)
    {
        s_Instance = nullptr;
    }
}

void ConsolePanel::OnImGuiRender(bool readOnly)
{
    if (!m_IsOpen)
    {
        return;
    }

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 10));
    ImGui::Begin(m_Name.c_str(), &m_IsOpen);
    ImGui::PushID(this);

    ImGui::BeginDisabled(readOnly);
    if (ImGui::Button("Clear"))
    {
        Clear();
    }
    ImGui::SameLine();

    const char* levels[] = {"TRACE", "DEBUG", "INFO", "WARNING", "ERROR", "FATAL", "NONE"};
    ImGui::SetNextItemWidth(150);
    if (ImGui::Combo("Log Level", &m_LogLevel, levels, IM_ARRAYSIZE(levels)))
    {
        SetTraceLogLevel(m_LogLevel);
    }
    ImGui::EndDisabled();

    ImGui::Separator();
    ImGui::Spacing();

    ImGui::BeginChild("ScrollingRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

    {
        std::lock_guard<std::mutex> lock(m_LogMutex);
        for (const auto& msg : m_Messages)
        {
            ImVec4 color = ImVec4(1, 1, 1, 1);
            if (msg.level == ConsoleLogLevel::Warn)
            {
                color = ImVec4(1, 1, 0, 1);
            }
            else if (msg.level == ConsoleLogLevel::Error)
            {
                color = ImVec4(1, 0, 0, 1);
            }

            ImGui::PushStyleColor(ImGuiCol_Text, color);
            ImGui::TextUnformatted(msg.message.c_str());
            ImGui::PopStyleColor();
        }
    }

    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
    {
        ImGui::SetScrollHereY(1.0f);
    }

    ImGui::EndChild();
    ImGui::PopID();
    ImGui::End();
    ImGui::PopStyleVar();
}

void ConsolePanel::Log(const std::string& message, ConsoleLogLevel level)
{
    if (!s_Instance)
    {
        return;
    }

    std::lock_guard<std::mutex> lock(m_LogMutex);
    ConsoleLogEntry entry;
    entry.level = level;
    entry.message = message;
    m_Messages.push_back(entry);

    if (m_Messages.size() > MAX_MESSAGES)
    {
        m_Messages.pop_front();
    }
}

void ConsolePanel::Clear()
{
    std::lock_guard<std::mutex> lock(m_LogMutex);
    m_Messages.clear();
}

void ConsolePanel::AddLog(const char* message, ConsoleLogLevel level)
{
    if (s_Instance)
    {
        s_Instance->Log(message, level);
    }
}
} // namespace CHEngine
