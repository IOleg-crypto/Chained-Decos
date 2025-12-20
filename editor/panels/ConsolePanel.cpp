//
// ConsolePanel.cpp - Console panel implementation
//

#include "ConsolePanel.h"
#include <chrono>
#include <ctime>
#include <imgui.h>


ConsolePanel::ConsolePanel(IEditor *editor) : m_editor(editor)
{
    Log("ChainedEditor initialized", LogMessage::Level::Info);
}

void ConsolePanel::Log(const std::string &message, LogMessage::Level level)
{
    LogMessage msg;
    msg.level = level;
    msg.message = message;

    // Get current time
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    char timeStr[32];
    std::strftime(timeStr, sizeof(timeStr), "[%H:%M:%S]", std::localtime(&time));
    msg.timestamp = timeStr;

    m_messages.push_back(msg);

    // Keep message count under limit
    while (m_messages.size() > MAX_MESSAGES)
    {
        m_messages.pop_front();
    }
}

void ConsolePanel::Clear()
{
    m_messages.clear();
}

void ConsolePanel::Render()
{
    if (!m_visible)
        return;

    if (ImGui::Begin("Console", &m_visible, ImGuiWindowFlags_MenuBar))
    {
        // Menu bar with filters
        if (ImGui::BeginMenuBar())
        {
            if (ImGui::SmallButton("Clear"))
            {
                Clear();
            }
            ImGui::Separator();
            ImGui::Checkbox("Info", &m_showInfo);
            ImGui::Checkbox("Warn", &m_showWarnings);
            ImGui::Checkbox("Error", &m_showErrors);
            ImGui::Separator();
            ImGui::Checkbox("Auto-scroll", &m_autoScroll);
            ImGui::EndMenuBar();
        }

        // Messages area
        ImGui::BeginChild("ScrollRegion", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()),
                          ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar);

        for (const auto &msg : m_messages)
        {
            // Filter by level
            if (msg.level == LogMessage::Level::Info && !m_showInfo)
                continue;
            if (msg.level == LogMessage::Level::Warning && !m_showWarnings)
                continue;
            if (msg.level == LogMessage::Level::Error && !m_showErrors)
                continue;

            // Color based on level
            ImVec4 color;
            const char *prefix;
            switch (msg.level)
            {
            case LogMessage::Level::Warning:
                color = ImVec4(1.0f, 0.8f, 0.2f, 1.0f);
                prefix = "[WARN]";
                break;
            case LogMessage::Level::Error:
                color = ImVec4(1.0f, 0.3f, 0.3f, 1.0f);
                prefix = "[ERR]";
                break;
            default:
                color = ImVec4(0.8f, 0.8f, 0.8f, 1.0f);
                prefix = "[INFO]";
                break;
            }

            ImGui::PushStyleColor(ImGuiCol_Text, color);
            ImGui::TextUnformatted((msg.timestamp + " " + prefix + " " + msg.message).c_str());
            ImGui::PopStyleColor();
        }

        // Auto-scroll to bottom
        if (m_autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
        {
            ImGui::SetScrollHereY(1.0f);
        }

        ImGui::EndChild();

        // Input line
        ImGui::Separator();
        bool reclaimFocus = false;
        ImGuiInputTextFlags inputFlags = ImGuiInputTextFlags_EnterReturnsTrue;
        if (ImGui::InputText("##input", m_inputBuffer, sizeof(m_inputBuffer), inputFlags))
        {
            if (m_inputBuffer[0] != '\0')
            {
                Log(std::string("> ") + m_inputBuffer, LogMessage::Level::Info);
                // TODO: Process command
                m_inputBuffer[0] = '\0';
            }
            reclaimFocus = true;
        }

        if (reclaimFocus)
        {
            ImGui::SetKeyboardFocusHere(-1);
        }
    }
    ImGui::End();
}
