#include "console_panel.h"
#include "imgui.h"


namespace CHEngine
{
ConsolePanel* ConsolePanel::s_Instance = nullptr;
std::deque<ConsoleLogEntry> ConsolePanel::s_Buffer;
std::mutex ConsolePanel::s_BufferMutex;

ConsolePanel::ConsolePanel()
{
    s_Instance = this;
    m_Name = "Console";

    // Flush static buffer to this instance
    std::lock_guard<std::mutex> lock(s_BufferMutex);
    while (!s_Buffer.empty())
    {
        m_Messages.push_back(std::move(s_Buffer.front()));
        s_Buffer.pop_front();
    }
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
    if (!m_IsOpen) return;

    ImGui::SetNextWindowSize(ImVec2(520, 600), ImGuiCond_FirstUseEver);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 10));
    
    if (ImGui::Begin(m_Name.c_str(), &m_IsOpen))
    {
        // Control Panel
        ImGui::BeginDisabled(readOnly);
        if (ImGui::Button("Clear")) Clear();
        ImGui::SameLine();
        
        ImGui::SetNextItemWidth(150);
        if (ImGui::InputTextWithHint("##filter", "Filter...", m_FilterBuffer, sizeof(m_FilterBuffer)))
        {
            // Rebuild visible indices on filter change
        }
        ImGui::SameLine();

        const char* levels[] = {"TRACE", "DEBUG", "INFO", "WARNING", "ERROR", "FATAL", "NONE"};
        ImGui::SetNextItemWidth(120);
        ImGui::Combo("Level", &m_LogLevel, levels, IM_ARRAYSIZE(levels));

        ImGui::EndDisabled();

        ImGui::Separator();

        // Rebuild visible indices if needed
        {
            std::lock_guard<std::mutex> lock(m_LogMutex);
            m_VisibleIndices.clear();
            std::string filterStr = m_FilterBuffer;
            std::transform(filterStr.begin(), filterStr.end(), filterStr.begin(), ::tolower);

            for (int i = 0; i < (int)m_Messages.size(); i++)
            {
                if (m_LogLevel != (int)ConsoleLogLevel::None && (int)m_Messages[i].level < m_LogLevel)
                    continue;

                if (!filterStr.empty())
                {
                    std::string msgLower = m_Messages[i].message;
                    std::transform(msgLower.begin(), msgLower.end(), msgLower.begin(), ::tolower);
                    if (msgLower.find(filterStr) == std::string::npos)
                        continue;
                }
                m_VisibleIndices.push_back(i);
            }
        }

        const float footer_height_to_reserve = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
        ImGui::BeginChild("ScrollingRegion", ImVec2(0, -footer_height_to_reserve), false, ImGuiWindowFlags_HorizontalScrollbar);

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
                    switch (msg.level) {
                        case ConsoleLogLevel::Trace: color = { 0.7f, 0.7f, 0.7f, 1.0f }; break;
                        case ConsoleLogLevel::Debug: color = { 0.2f, 0.7f, 0.9f, 1.0f }; break;
                        case ConsoleLogLevel::Warn:  color = { 1.0f, 0.8f, 0.0f, 1.0f }; break;
                        case ConsoleLogLevel::Error: color = { 1.0f, 0.2f, 0.2f, 1.0f }; break;
                        case ConsoleLogLevel::Fatal: color = { 1.0f, 0.0f, 1.0f, 1.0f }; break;
                        default:                     color = { 1.0f, 1.0f, 1.0f, 1.0f }; break;
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
            ImGui::SetScrollHereY(1.0f);
        m_ScrollToBottom = false;

        ImGui::EndChild();
    }
    ImGui::End();
    ImGui::PopStyleVar();
}
void ConsolePanel::Log(const std::string& message, ConsoleLogLevel level)
{
    std::lock_guard<std::mutex> lock(m_LogMutex);
    
    // Форматуємо час (наприклад, "14:20:05")
    std::string timeStr = GetCurrentTimestamp(); 
    
    m_Messages.push_back({ level, message, timeStr });
    
    if (m_Messages.size() > MAX_MESSAGES)
        m_Messages.pop_front();

    m_ScrollToBottom = true; // Сигнал для OnImGuiRender
}

void ConsolePanel::Clear()
{
    std::lock_guard<std::mutex> lock(m_LogMutex);
    m_Messages.clear();
}

void ConsolePanel::AddLog(const char* message, int level)
{
    if (s_Instance)
    {
        s_Instance->Log(message, (ConsoleLogLevel)level);
    }
    else
    {
        // Buffer logs until ConsolePanel is initialized
        std::lock_guard<std::mutex> lock(s_BufferMutex);
        if (s_Buffer.size() < MAX_MESSAGES)
        {
            s_Buffer.push_back({ (ConsoleLogLevel)level, message, GetCurrentTimestamp() });
        }
    }
}

std::string ConsolePanel::GetCurrentTimestamp()
{
    using namespace std::chrono;
    
    auto now = system_clock::now();
    auto in_time_t = system_clock::to_time_t(now);

    std::tm time_info;
#if defined(_MSC_VER)
    localtime_s(&time_info, &in_time_t);
#elif defined(_WIN32)
    std::tm* tm_ptr = std::localtime(&in_time_t);
    if (tm_ptr) time_info = *tm_ptr;
#else
    localtime_r(&in_time_t, &time_info);
#endif

    std::stringstream ss;
    ss << "[" << std::put_time(&time_info, "%H:%M:%S") << "] ";
    return ss.str();
}

} // namespace CHEngine
