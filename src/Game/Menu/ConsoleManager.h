/**
 * @file ConsoleManager.h
 * @brief In-game console system for debugging and cheats
 *
 * The ConsoleManager class provides an in-game console interface
 * for executing commands, viewing output, and debugging game state.
 */

#ifndef CONSOLE_MANAGER_H
#define CONSOLE_MANAGER_H

#include <string>
#include <vector>
#include "MenuConstants.h"

namespace MenuConstants {
    using namespace MenuConstants;
}

/**
 * @class ConsoleManager
 * @brief Manages the in-game console functionality
 *
 * This class handles console input, output, command processing,
 * and rendering of the console interface.
 */
class ConsoleManager {
private:
    bool consoleOpen = false;
    std::string consoleInput;
    std::vector<std::string> consoleHistory;
    std::vector<std::string> consoleOutput;
    size_t consoleHistoryIndex = 0;

    Font consoleFont;

    // Console dimensions and layout
    int consoleHeight = 200;
    int inputHeight = 30;
    int lineHeight = 20;

    // Scrolling
    int scrollOffset = 0;

public:
    /**
     * @brief Default constructor that initializes the console font and settings
     */
    ConsoleManager();

    // Console state management
    void ToggleConsole();
    void OpenConsole();
    void CloseConsole();
    bool IsConsoleOpen() const { return consoleOpen; }

    // Input handling
    void HandleInput();
    void ProcessInputCharacter(char character);
    void ProcessBackspace();
    void ProcessEnter();
    void NavigateHistory(bool up);

    // Command execution
    void ExecuteCommand(const std::string& command);
    void AddOutput(const std::string& text);
    void ClearOutput();

    // Rendering
    void RenderConsole() const;

    // History management
    void AddToHistory(const std::string& command);
    const std::vector<std::string>& GetH istory() const { return consoleHistory; }
    const std::vector<std::string>& GetOutput() const { return consoleOutput; }

    // Utility
    std::string GetCurrentInput() const { return consoleInput; }
    void SetInput(const std::string& input) { consoleInput = input; }
    int GetVisibleLineCount() const;
    void ScrollUp();
    void ScrollDown();

private:
    // Input processing helpers
    void ProcessCommand(const std::string& command);
    void AddToCommandHistory(const std::string& command);
    void UpdateScrollPosition();

    // Rendering helpers
    void RenderOutputArea() const;
    void RenderInputArea() const;
    void RenderScrollbar() const;

private:
    // Command processing
    void ProcessHelpCommand();
    void ProcessClearCommand();
    void ProcessSetCommand(const std::string& key, const std::string& value);
    void ProcessGetCommand(const std::string& key);

    // Built-in commands
    std::vector<std::string> GetAvailableCommands() const;
};

#endif // CONSOLE_MANAGER_H