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
/**
 * @class ConsolePanel
 * @brief A developer tool panel that displays system log messages in real-time using ImGui.
 * 
 * This panel supports live filtering by log severity levels (LogLevel) and text substrings,
 * handles automatic scrolling, and ensures thread-safe access to underlying log entries.
 */
class ConsolePanel : public Panel
{
public:
    /**
     * @brief Constructs the ConsolePanel and initializes the UI state.
     */
    ConsolePanel();

    /**
     * @brief Destructs the ConsolePanel and releases its resources.
     */
    ~ConsolePanel();

    /**
     * @brief Renders the console interface using ImGui commands.
     * @param readOnly If true, disables control elements such as the clear button and filters input.
     */
    void OnImGuiRender(bool readOnly = false) override;

    /**
     * @brief Clears all accumulated log messages from the buffer and resets visible indices.
     */
    void Clear();

private:
    /// @brief Thread-safe double-ended queue storing raw log entries. Deque enables efficient pop_front when exceeding limits.
    std::deque<BufferedLogEntry> m_Messages;
    
    /// @brief Cached indices of messages from m_Messages that successfully passed active filters. Optimizes render passes.
    std::vector<int> m_VisibleIndices; 
    
    /// @brief Mutex to guarantee thread-safe operations when background threads submit logs while the main thread renders them.
    std::mutex m_LogMutex;

    // --- UI State ---
    
    /// @brief Currently selected minimum severity level cutoff for rendering messages.
    int m_LogLevel = (int)LogLevel::LogInfo;
    
    /// @brief Flag to trigger the ImGui container window to scroll down to the latest message.
    bool m_ScrollToBottom = false;
    
    /// @brief Character buffer containing the text substring query for filtering.
    char m_FilterBuffer[128] = {0};

    /// @brief The hard limit of history retention. Prevents memory leaks by discarding the oldest logs upon overflow.
    const size_t MAX_MESSAGES = 25000;
};
} // namespace Chained

#endif // CH_CONSOLE_PANEL_H