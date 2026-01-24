#ifndef CH_CONSOLE_PANEL_H
#define CH_CONSOLE_PANEL_H

#include <cstdint>
#include <deque>
#include <mutex>
#include <string>
#include <vector>

#include "panel.h" // Added include for Panel

namespace CHEngine
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

class ConsolePanel : public Panel // Inherited from Panel
{
public:
    ConsolePanel();
    ~ConsolePanel() = default;

    void OnImGuiRender(bool readOnly = false) override; // Added override keyword
    void Log(const std::string &message, ConsoleLogLevel level = ConsoleLogLevel::Info);
    void Clear();

    static void AddLog(const char *message, ConsoleLogLevel level = ConsoleLogLevel::Info);

private:
    static ConsolePanel *s_Instance;
    std::deque<ConsoleLogEntry> m_Messages;
    std::mutex m_LogMutex;
    static constexpr size_t MAX_MESSAGES = 500;
};
} // namespace CHEngine

#endif // CH_CONSOLE_PANEL_H
