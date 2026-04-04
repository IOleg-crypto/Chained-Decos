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

    static void AddLog(const char* message, ConsoleLogLevel level = ConsoleLogLevel::Info);

    static ConsolePanel* s_Instance;

private:
    
    std::deque<ConsoleLogEntry> m_Messages;
    std::mutex m_LogMutex;

    // Стан інтерфейсу
    int m_LogLevel = (int)ConsoleLogLevel::Info; 
    bool m_ScrollToBottom = false;
    char m_FilterBuffer[256] = { 0 }; // Буфер для пошуку

    static constexpr size_t MAX_MESSAGES = 10000;

    // Допоміжний метод для отримання поточного часу
    std::string GetCurrentTimestamp();
};
} // namespace CHEngine

#endif // CH_CONSOLE_PANEL_H