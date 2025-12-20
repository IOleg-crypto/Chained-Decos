//
// ConsolePanel.h - Console/Log output panel
//

#ifndef CONSOLEPANEL_H
#define CONSOLEPANEL_H

#include "IEditorPanel.h"
#include <deque>
#include <string>
#include <vector>


class IEditor;

// Log message with severity level
struct LogMessage
{
    enum class Level
    {
        Info,
        Warning,
        Error
    };
    Level level;
    std::string message;
    std::string timestamp;
};

// Displays log messages and allows command input
class ConsolePanel : public IEditorPanel
{
public:
    explicit ConsolePanel(IEditor *editor);
    ~ConsolePanel() override = default;

    // IEditorPanel interface
    void Render() override;
    const char *GetName() const override
    {
        return "Console";
    }
    const char *GetDisplayName() const override
    {
        return "Console";
    }
    bool IsVisible() const override
    {
        return m_visible;
    }
    void SetVisible(bool visible) override
    {
        m_visible = visible;
    }

    // Console-specific
    void Log(const std::string &message, LogMessage::Level level = LogMessage::Level::Info);
    void Clear();

private:
    IEditor *m_editor;
    bool m_visible = true;
    std::deque<LogMessage> m_messages;
    static constexpr size_t MAX_MESSAGES = 500;
    bool m_autoScroll = true;
    bool m_showInfo = true;
    bool m_showWarnings = true;
    bool m_showErrors = true;
    char m_inputBuffer[256] = {0};
};

#endif // CONSOLEPANEL_H
