#ifndef CH_CONSOLE_PANEL_H
#define CH_CONSOLE_PANEL_H

#include "panel.h"
#include "engine/core/log.h"
#include <deque>
#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

namespace CHEngine
{
class ConsolePanel : public Panel
{
public:
    ConsolePanel();
    ~ConsolePanel();

    void OnImGuiRender(bool readOnly = false) override;
    void Clear();

private:
    
    std::deque<BufferedLogEntry> m_Messages;
    std::vector<int> m_VisibleIndices; // Indices of messages that pass the filter
    std::mutex m_LogMutex;

    // UI State
    int m_LogLevel = (int)LogLevel::LogInfo; 
    bool m_ScrollToBottom = false;
    char m_FilterBuffer[128] = { 0 }; 

    const size_t MAX_MESSAGES = 10000;
};
} // namespace CHEngine

#endif // CH_CONSOLE_PANEL_H