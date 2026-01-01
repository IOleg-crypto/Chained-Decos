#pragma once
#include "editor/panels/EditorPanel.h"
#include <cstdint>
#include <deque>
#include <string>

namespace CHEngine
{
struct LogMessage
{
    enum class Level : uint8_t
    {
        Info,
        Warn,
        Error
    };
    Level level;
    std::string message;
    std::string timestamp;
};

/**
 * @brief Panel for displaying engine and editor logs
 */
class ConsolePanel : public EditorPanel
{
public:
    ConsolePanel();
    virtual ~ConsolePanel();

    // --- Panel Lifecycle ---
public:
    virtual void OnImGuiRender() override;

    // --- Logging API ---
public:
    void Log(const std::string &message, LogMessage::Level level = LogMessage::Level::Info);
    void Clear();
    static void AddLog(const char *message, LogMessage::Level level = LogMessage::Level::Info);

    // --- Member Variables ---
private:
    static ConsolePanel *s_Instance;
    std::deque<LogMessage> m_Messages;
    static constexpr size_t MAX_MESSAGES = 500;

    bool m_AutoScroll = true;
    bool m_ShowInfo = true;
    bool m_ShowWarnings = true;
    bool m_ShowErrors = true;

    char m_InputBuffer[256] = {0};
};
} // namespace CHEngine
