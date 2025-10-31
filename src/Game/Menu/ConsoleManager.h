/**
 * @file ConsoleManager.h
 * In-game console system for debugging and cheats.
 *
 * The ConsoleManager class provides an in-game console interface
 * for executing commands, viewing output, and debugging game state.
 */

#ifndef CONSOLE_MANAGER_H
#define CONSOLE_MANAGER_H

#include <string>
#include <vector>
#include <raylib.h>
#include <imgui/imgui.h>
#include "MenuConstants.h"

namespace MenuConstants {
    using namespace MenuConstants;
}

// Forward declaration - will be included in .cpp
class Game;

/**
 * @class ConsoleManager
 * Manages the in-game console functionality.
 *
 * This class handles console input, output, command processing,
 * and rendering of the console interface.
 */
class ConsoleManager {
private:
    Game* m_game;
    bool consoleOpen = false;
    std::vector<std::string> consoleHistory;
    std::vector<std::string> consoleOutput;

    // Console dimensions and layout (not used with ImGui)

    // Constants
    static constexpr size_t MAX_CONSOLE_LINES = 100;
    static constexpr size_t MAX_HISTORY_LINES = 50;

public:
    /**
     * @brief Constructor that initializes the console font and settings
     * @param game Pointer to Game instance for command execution
     */
    ConsoleManager(Game* game);

    // Console state management
    void ToggleConsole();
    void OpenConsole();
    void CloseConsole();
    bool IsConsoleOpen() const { return consoleOpen; }

    // Input handling removed as ImGui handles it

    // Command execution
    void ExecuteCommand(const std::string& command);
    void AddOutput(const std::string& text);
    void ClearOutput();

    // Rendering
    void RenderConsole();

    // History management
    void AddToHistory(const std::string& command);
    const std::vector<std::string>& GetHistory() const { return consoleHistory; }
    const std::vector<std::string>& GetOutput() const { return consoleOutput; }

    // Utility (removed as ImGui handles input)

private:
    // Command processing helpers removed

private:
    // Command processing
    void ProcessHelpCommand();
    void ProcessClearCommand();
    void ProcessSetCommand(const std::string& key, const std::string& value);
    void ProcessGetCommand(const std::string& key);
    void ProcessNoclipCommand();

    // Built-in commands
    std::vector<std::string> GetAvailableCommands() const;
};

#endif // CONSOLE_MANAGER_H