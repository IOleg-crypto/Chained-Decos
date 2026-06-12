#ifndef CH_CONSOLE_PANEL_H
#define CH_CONSOLE_PANEL_H

#include "engine/core/log.h"
#include "panel.h"
#include <cstdint>
#include <deque>
#include <mutex>
#include <string>
#include <vector>


namespace Chained
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
    char m_FilterBuffer[128] = {0};

    const size_t MAX_MESSAGES = 10000;
};
} // namespace Chained

#endif // CH_CONSOLE_PANEL_H