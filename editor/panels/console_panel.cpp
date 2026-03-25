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
    if (!m_IsOpen) return;

    ImGui::SetNextWindowSize(ImVec2(520, 600), ImGuiCond_FirstUseEver);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 10));
    
    if (ImGui::Begin(m_Name.c_str(), &m_IsOpen))
    {
        // Панель керування
        ImGui::BeginDisabled(readOnly);
        if (ImGui::Button("Clear")) Clear();
        ImGui::SameLine();
        
        static char filter[128] = "";
        ImGui::SetNextItemWidth(150);
        ImGui::InputTextWithHint("##filter", "Filter...", filter, IM_ARRAYSIZE(filter));
        ImGui::SameLine();

        const char* levels[] = {"TRACE", "DEBUG", "INFO", "WARNING", "ERROR", "FATAL", "NONE"};
        ImGui::SetNextItemWidth(120);
        if (ImGui::Combo("Level", &m_LogLevel, levels, IM_ARRAYSIZE(levels))) {
            SetTraceLogLevel(m_LogLevel);
        }
        ImGui::EndDisabled();

        ImGui::Separator();

        // Область тексту з використанням Clipper для швидкодії
        const float footer_height_to_reserve = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
        ImGui::BeginChild("ScrollingRegion", ImVec2(0, -footer_height_to_reserve), false, ImGuiWindowFlags_HorizontalScrollbar);

        {
            std::lock_guard<std::mutex> lock(m_LogMutex);
            
            // Використовуємо Clipper: він пропустить рендер повідомлень поза екраном
            ImGuiListClipper clipper;
            clipper.Begin((int)m_Messages.size());
            
            while (clipper.Step())
            {
                for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
                {
                    const auto& msg = m_Messages[i];
                    
                    // Фільтрація (проста реалізація)
                    if (filter[0] != '\0' && msg.message.find(filter) == std::string::npos)
                        continue;

                    ImVec4 color;
                    switch (msg.level) {
                        case ConsoleLogLevel::Trace: color = { 0.7f, 0.7f, 0.7f, 1.0f }; break; // Сірий
                        case ConsoleLogLevel::Debug: color = { 0.2f, 0.7f, 0.9f, 1.0f }; break; // Блакитний
                        case ConsoleLogLevel::Warn:  color = { 1.0f, 0.8f, 0.0f, 1.0f }; break; // Жовтий
                        case ConsoleLogLevel::Error: color = { 1.0f, 0.2f, 0.2f, 1.0f }; break; // Червоний
                        case ConsoleLogLevel::Fatal: color = { 1.0f, 0.0f, 1.0f, 1.0f }; break; // Пурпурний
                        default:                     color = { 1.0f, 1.0f, 1.0f, 1.0f }; break; // Білий
                    }

                    ImGui::PushStyleColor(ImGuiCol_Text, color);
                    ImGui::TextUnformatted(msg.message.c_str());
                    ImGui::PopStyleColor();
                }
            }
        }

        // Авто-скрол до останнього повідомлення
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

void ConsolePanel::AddLog(const char* message, ConsoleLogLevel level)
{
    if (s_Instance)
    {
        s_Instance->Log(message, level);
    }
}

std::string ConsolePanel::GetCurrentTimestamp()
{
    using namespace std::chrono;
    
    // Отримуємо поточний час
    auto now = system_clock::now();
    auto in_time_t = system_clock::to_time_t(now);

    std::stringstream ss;
    // Форматуємо час: [14:20:05]
    ss << "[" << std::put_time(std::localtime(&in_time_t), "%H:%M:%S") << "] ";
    return ss.str();
}
} // namespace CHEngine
