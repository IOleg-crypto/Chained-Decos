#ifndef CONSOLEPANEL_H
#define CONSOLEPANEL_H

#include <cstdint> // Required for uint8_t
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

class ConsolePanel
{
public:
    ConsolePanel();
    virtual ~ConsolePanel();

    void OnImGuiRender();

    void Log(const std::string &message, LogMessage::Level level = LogMessage::Level::Info);
    void Clear();

    static void AddLog(const char *message, LogMessage::Level level = LogMessage::Level::Info);

    bool IsVisible() const
    {
        return m_isVisible;
    }
    void SetVisible(bool visible)
    {
        m_isVisible = visible;
    }

private:
    static ConsolePanel *s_Instance;
    std::deque<LogMessage> m_Messages;
    static constexpr size_t MAX_MESSAGES = 500;
    bool m_AutoScroll = true;
    bool m_ShowInfo = true;
    bool m_ShowWarnings = true;
    bool m_ShowErrors = true;
    char m_InputBuffer[256] = {0};
    bool m_isVisible = true;
};
} // namespace CHEngine
#endif
