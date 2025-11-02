#ifndef CONSOLE_MANAGER_H
#define CONSOLE_MANAGER_H

#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
#include <raylib.h>
#include <imgui/imgui.h>
#include "MenuConstants.h"

// Forward declaration to break circular dependency
class Game;

// Command callback function type
using CommandCallback = std::function<void(const std::vector<std::string>&, class ConsoleManager*)>;

// Command information structure
struct CommandInfo {
    std::string name;
    std::string fullName;  // Full name with prefix (e.g., "player.pos")
    std::string category;   // Category/prefix (e.g., "player", "engine")
    std::string description;
    std::string usage;
    CommandCallback callback;
    
    CommandInfo() = default;
    CommandInfo(const std::string& n, const std::string& full, const std::string& cat,
                const std::string& desc, const std::string& use, CommandCallback cb)
        : name(n), fullName(full), category(cat), description(desc), usage(use), callback(std::move(cb)) {}
};

class ConsoleManager {
private:
    Game* m_game;
    bool consoleOpen = false;
    std::vector<std::string> consoleHistory;
    std::vector<std::string> consoleOutput;

    // Command registry
    std::unordered_map<std::string, CommandInfo> m_commands;

    // Constants
    static constexpr size_t MAX_CONSOLE_LINES = 100;
    static constexpr size_t MAX_HISTORY_LINES = 50;

public:
    ConsoleManager(Game* game);

    // Console state management
    void ToggleConsole();
    void OpenConsole();
    void CloseConsole();
    bool IsConsoleOpen() const { return consoleOpen; }

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
    
    // Clipboard operations
    void CopyToClipboard(const std::string& text);
    void CopyLastCommand();
    std::string GetLastCommand() const;

    // Command registration
    void RegisterCommand(const std::string& name, const std::string& description, 
                       const std::string& usage, CommandCallback callback);
    void RegisterCommandWithPrefix(const std::string& category, const std::string& name,
                                  const std::string& description, const std::string& usage,
                                  CommandCallback callback, bool alsoRegisterWithoutPrefix = true);
    void UnregisterCommand(const std::string& name);
    const CommandInfo* GetCommandInfo(const std::string& name) const;
    std::vector<std::string> GetAvailableCommandNames() const;
    std::vector<std::string> GetCommandsByCategory(const std::string& category) const;
    std::vector<std::string> GetAvailableCategories() const;

    // Helper to get Game instance
    Game* GetGame() const { return m_game; }

private:
    // Initialize built-in commands
    void RegisterBuiltinCommands();
    
    // Command argument parsing
    std::vector<std::string> ParseArguments(const std::string& args) const;
    
    // Command lookup with prefix support
    const CommandInfo* FindCommand(const std::string& cmdName) const;
};

#endif // CONSOLE_MANAGER_H