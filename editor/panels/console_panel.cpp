#include "console_panel.h"
#include "imgui.h"
#include <algorithm>
#include <cctype>

namespace CHEngine
{
ConsolePanel::ConsolePanel()
{
    m_Name = "Console";

    auto bufferedMessages = Log::ConsumeBufferedMessages();
    for (auto& entry : bufferedMessages)
    {
        m_Messages.push_back(std::move(entry));
    }

    m_ScrollToBottom = !m_Messages.empty();
}

ConsolePanel::~ConsolePanel()
{
}

void ConsolePanel::OnImGuiRender(bool readOnly)
{
    if (!m_IsOpen)
    {
        return;
    }

    ImGui::SetNextWindowSize(ImVec2(520, 600), ImGuiCond_FirstUseEver);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 10));

    if (ImGui::Begin(m_Name.c_str(), &m_IsOpen))
    {
        // Control Panel
        ImGui::BeginDisabled(readOnly);
        if (ImGui::Button("Clear"))
        {
            Clear();
        }
        ImGui::SameLine();

        ImGui::SetNextItemWidth(150);
        ImGui::InputTextWithHint("##filter", "Filter...", m_FilterBuffer, sizeof(m_FilterBuffer));
        ImGui::SameLine();

        const char* levels[] = {"TRACE", "INFO", "WARNING", "ERROR", "FATAL", "NONE"};
        ImGui::SetNextItemWidth(120);
        ImGui::Combo("Level", &m_LogLevel, levels, IM_ARRAYSIZE(levels));

        ImGui::EndDisabled();

        ImGui::Separator();

        auto bufferedMessages = Log::ConsumeBufferedMessages();
        if (!bufferedMessages.empty())
        {
            std::lock_guard<std::mutex> lock(m_LogMutex);

            for (auto& entry : bufferedMessages)
            {
                m_Messages.push_back(std::move(entry));
            }

            while (m_Messages.size() > MAX_MESSAGES)
            {
                m_Messages.pop_front();
            }

            m_ScrollToBottom = true;
        }

        // Rebuild visible indices if needed
        {
            std::lock_guard<std::mutex> lock(m_LogMutex);
            m_VisibleIndices.clear();
            std::string filterStr = m_FilterBuffer;
            std::transform(filterStr.begin(), filterStr.end(), filterStr.begin(), [](unsigned char ch) {
                return static_cast<char>(std::tolower(ch));
            });

            for (int i = 0; i < (int)m_Messages.size(); i++)
            {
                if (m_LogLevel != (int)LogLevel::LogNone && (int)m_Messages[i].level < m_LogLevel)
                {
                    continue;
                }

                if (!filterStr.empty())
                {
                    std::string msgLower = m_Messages[i].message;
                    std::transform(msgLower.begin(), msgLower.end(), msgLower.begin(), [](unsigned char ch) {
                        return static_cast<char>(std::tolower(ch));
                    });
                    if (msgLower.find(filterStr) == std::string::npos)
                    {
                        continue;
                    }
                }
                m_VisibleIndices.push_back(i);
            }
        }

        const float footer_height_to_reserve = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
        ImGui::BeginChild("ScrollingRegion", ImVec2(0, -footer_height_to_reserve), false,
                          ImGuiWindowFlags_HorizontalScrollbar);

        {
            std::lock_guard<std::mutex> lock(m_LogMutex);

            ImGuiListClipper clipper;
            clipper.Begin((int)m_VisibleIndices.size());

            while (clipper.Step())
            {
                for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
                {
                    int msgIdx = m_VisibleIndices[i];
                    const auto& msg = m_Messages[msgIdx];

                    ImVec4 color;
                    switch (msg.level)
                    {
                    case LogLevel::LogTrace:
                        color = {0.7f, 0.7f, 0.7f, 1.0f};
                        break;
                    case LogLevel::LogInfo:
                        color = {1.0f, 1.0f, 1.0f, 1.0f};
                        break;
                    case LogLevel::LogWarning:
                        color = {1.0f, 0.8f, 0.0f, 1.0f};
                        break;
                    case LogLevel::LogError:
                        color = {1.0f, 0.2f, 0.2f, 1.0f};
                        break;
                    case LogLevel::LogFatal:
                        color = {1.0f, 0.0f, 1.0f, 1.0f};
                        break;
                    default:
                        color = {1.0f, 1.0f, 1.0f, 1.0f};
                        break;
                    }

                    // Timestamp
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
                    ImGui::TextUnformatted(msg.timestamp.c_str());
                    ImGui::PopStyleColor();
                    ImGui::SameLine();

                    ImGui::PushStyleColor(ImGuiCol_Text, color);
                    ImGui::TextUnformatted(msg.message.c_str());
                    ImGui::PopStyleColor();
                }
            }
        }

        if (m_ScrollToBottom || (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()))
        {
            ImGui::SetScrollHereY(1.0f);
        }
        m_ScrollToBottom = false;

        ImGui::EndChild();
    }
    ImGui::End();
    ImGui::PopStyleVar();
}

void ConsolePanel::Clear()
{
    std::lock_guard<std::mutex> lock(m_LogMutex);
    m_Messages.clear();
}

} // namespace CHEngine
