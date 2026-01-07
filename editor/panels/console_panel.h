#ifndef CH_CONSOLE_PANEL_H
#define CH_CONSOLE_PANEL_H

#include <cstdint>
#include <deque>
#include <string>
#include <vector>

namespace CH
{
enum class ConsoleLogLevel : uint32_t
{
    Info = 0,
    Warn = 1,
    Error = 2
};

struct ConsoleLogEntry
{
    ConsoleLogLevel level;
    std::string message;
};

class ConsolePanel
{
public:
    ConsolePanel();
    ~ConsolePanel() = default;

    void OnImGuiRender(bool readOnly = false);
    void Log(const std::string &message, ConsoleLogLevel level = ConsoleLogLevel::Info);
    void Clear();

    static void AddLog(const char *message, ConsoleLogLevel level = ConsoleLogLevel::Info);

private:
    static ConsolePanel *s_Instance;
    std::deque<ConsoleLogEntry> m_Messages;
    static constexpr size_t MAX_MESSAGES = 500;
};
} // namespace CH

#endif // CH_CONSOLE_PANEL_H
