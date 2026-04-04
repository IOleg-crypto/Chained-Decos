#ifndef CH_CONSOLE_PANEL_H
#define CH_CONSOLE_PANEL_H

#include <deque>
#include <cstdint>
#include <mutex>
#include <string>
#include <vector>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <ctime>

#include "panel.h"

namespace CHEngine
{
// Extend log levels for engine/editor compatibility

enum class ConsoleLogLevel : uint32_t
{
    Trace = 0,
    Debug,
    Info,
    Warn,
    Error,
    Fatal,
    None
};

struct ConsoleLogEntry
{
    ConsoleLogLevel level;
    std::string message;
    std::string timestamp; // Додаємо час для зручності
};

class ConsolePanel : public Panel
{
public:
    ConsolePanel();
    ~ConsolePanel();

    void OnImGuiRender(bool readOnly = false) override;
    void Log(const std::string& message, ConsoleLogLevel level = ConsoleLogLevel::Info);
    void Clear();

    static void AddLog(const char* message, int level = (int)ConsoleLogLevel::Info);

    static ConsolePanel* s_Instance;
    static std::deque<ConsoleLogEntry> s_Buffer;
    static std::mutex s_BufferMutex;

private:
    
    std::deque<ConsoleLogEntry> m_Messages;
    std::vector<int> m_VisibleIndices; // Indices of messages that pass the filter
    std::mutex m_LogMutex;

    // UI State
    int m_LogLevel = (int)ConsoleLogLevel::Info; 
    bool m_ScrollToBottom = false;
    char m_FilterBuffer[128] = { 0 }; 

    static constexpr size_t MAX_MESSAGES = 10000;

    // Допоміжний метод для отримання поточного часу
    static std::string GetCurrentTimestamp();
};
} // namespace CHEngine

#endif // CH_CONSOLE_PANEL_H