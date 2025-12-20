#include "ConsoleManager.h"
#include "core/Engine.h"
#include "core/application/EngineApplication.h"
#include "scene/main/core/LevelManager.h"

#include "project/chaineddecos/player/core/player.h"
#include <algorithm>
#include <cctype>
#include <imgui.h>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <raylib.h>
#include <sstream>
#include <unordered_set>

ConsoleManager::ConsoleManager()
{
    TraceLog(LOG_INFO, "ConsoleManager::ConsoleManager() - CONSOLE MANAGER INITIALIZED");

    // Register all built-in commands
    RegisterBuiltinCommands();

    TraceLog(LOG_INFO, "ConsoleManager::ConsoleManager() - Registered %zu commands",
             m_commands.size());
}

IPlayer *ConsoleManager::GetPlayer()
{
    // Get Player through Engine -> PlayerService
    auto player = Engine::Instance().GetPlayer();
    return player.get();
}

IEngine *ConsoleManager::GetEngine() const
{
    return &Engine::Instance();
}

void ConsoleManager::ToggleConsole()
{
    consoleOpen = !consoleOpen;
}

void ConsoleManager::OpenConsole()
{
    consoleOpen = true;
}

void ConsoleManager::CloseConsole()
{
    consoleOpen = false;
}

// Input handling removed as ImGui handles it

void ConsoleManager::ExecuteCommand(const std::string &command)
{
    AddOutput("> " + command);

    std::istringstream iss(command);
    std::string cmd;
    iss >> cmd;

    std::string args;
    std::getline(iss, args);

    // Remove leading whitespace from args
    args.erase(0, args.find_first_not_of(" \t"));

    if (cmd.empty())
    {
        return;
    }

    // Convert to lowercase for case-insensitive matching
    std::string cmdLower = cmd;
    std::transform(cmdLower.begin(), cmdLower.end(), cmdLower.begin(), ::tolower);

    // Look up command (supports prefixes like "player.pos")
    const CommandInfo *cmdInfo = FindCommand(cmdLower);
    if (cmdInfo)
    {
        std::vector<std::string> parsedArgs = ParseArguments(args);
        try
        {
            cmdInfo->callback(parsedArgs, this);
        }
        catch (const std::exception &e)
        {
            AddOutput("Error executing command: " + std::string(e.what()));
        }
    }
    else
    {
        AddOutput("Unknown command: " + cmd + ". Type 'help' for available commands.");
    }
}

std::vector<std::string> ConsoleManager::ParseArguments(const std::string &args) const
{
    std::vector<std::string> result;
    if (args.empty())
    {
        return result;
    }

    std::istringstream iss(args);
    std::string arg;
    while (iss >> arg)
    {
        result.push_back(arg);
    }
    return result;
}

void ConsoleManager::RegisterCommand(const std::string &name, const std::string &description,
                                     const std::string &usage, CommandCallback callback)
{
    std::string nameLower = name;
    std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(), ::tolower);

    m_commands[nameLower] =
        CommandInfo(nameLower, nameLower, "", description, usage, std::move(callback));
    TraceLog(LOG_DEBUG, "Registered console command: %s", nameLower.c_str());
}

const CommandInfo *ConsoleManager::FindCommand(const std::string &cmdName) const
{
    // First, try exact match
    auto it = m_commands.find(cmdName);
    if (it != m_commands.end())
    {
        return &it->second;
    }

    // If not found and contains dot, it might be a prefix command
    // (already handled by exact match above)

    return nullptr;
}

void ConsoleManager::UnregisterCommand(const std::string &name)
{
    std::string nameLower = name;
    std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(), ::tolower);
    m_commands.erase(nameLower);
}

const CommandInfo *ConsoleManager::GetCommandInfo(const std::string &name) const
{
    return FindCommand(name);
}

std::vector<std::string> ConsoleManager::GetAvailableCommandNames() const
{
    std::vector<std::string> names;
    std::unordered_set<std::string> seen;

    for (const auto &pair : m_commands)
    {
        // Add only fullName (with prefix) to avoid duplicates
        if (seen.insert(pair.second.fullName).second)
        {
            names.push_back(pair.second.fullName);
        }
    }
    std::sort(names.begin(), names.end());
    return names;
}

std::vector<std::string> ConsoleManager::GetCommandsByCategory(const std::string &category) const
{
    std::vector<std::string> commands;
    std::string catLower = category;
    std::transform(catLower.begin(), catLower.end(), catLower.begin(), ::tolower);

    for (const auto &pair : m_commands)
    {
        if (pair.second.category == catLower)
        {
            commands.push_back(pair.second.fullName);
        }
    }
    std::sort(commands.begin(), commands.end());
    return commands;
}

std::vector<std::string> ConsoleManager::GetAvailableCategories() const
{
    std::unordered_set<std::string> categories;
    for (const auto &pair : m_commands)
    {
        if (!pair.second.category.empty())
        {
            categories.insert(pair.second.category);
        }
    }
    std::vector<std::string> result(categories.begin(), categories.end());
    std::sort(result.begin(), result.end());
    return result;
}

void ConsoleManager::RegisterBuiltinCommands()
{
    // Help command
    RegisterCommand(
        "help", "Show available commands", "help [command] [category]",
        [](const std::vector<std::string> &args, ConsoleManager *console)
        {
            if (args.empty())
            {
                console->AddOutput("Available commands by category:");
                console->AddOutput("");

                // Show commands grouped by category (Source Engine style)
                auto categories = console->GetAvailableCategories();
                for (const auto &category : categories)
                {
                    console->AddOutput("[" + category + "]");
                    auto commands = console->GetCommandsByCategory(category);
                    for (const auto &cmdName : commands)
                    {
                        const CommandInfo *info = console->FindCommand(cmdName);
                        if (info)
                        {
                            console->AddOutput("  " + info->fullName + " - " + info->description);
                        }
                    }
                    console->AddOutput("");
                }

                // Show commands without category
                auto allCommands = console->GetAvailableCommandNames();
                bool hasGeneralCommands = false;
                for (const auto &cmdName : allCommands)
                {
                    const CommandInfo *info = console->FindCommand(cmdName);
                    if (info && info->category.empty())
                    {
                        if (!hasGeneralCommands)
                        {
                            console->AddOutput("[general]");
                            hasGeneralCommands = true;
                        }
                        console->AddOutput("  " + info->name + " - " + info->description);
                    }
                }
                if (hasGeneralCommands)
                {
                    console->AddOutput("");
                }

                console->AddOutput("");
                console->AddOutput("Type 'help <command>' for detailed usage.");
                console->AddOutput("Type 'help <category>' to see commands in a category.");
            }
            else
            {
                // Check if it's a category or a command
                std::string arg = args[0];
                std::transform(arg.begin(), arg.end(), arg.begin(), ::tolower);

                auto categories = console->GetAvailableCategories();
                if (std::find(categories.begin(), categories.end(), arg) != categories.end())
                {
                    // It's a category
                    console->AddOutput("Commands in category [" + arg + "]:");
                    auto commands = console->GetCommandsByCategory(arg);
                    for (const auto &cmdName : commands)
                    {
                        const CommandInfo *info = console->FindCommand(cmdName);
                        if (info)
                        {
                            console->AddOutput("  " + info->fullName + ": " + info->description);
                            console->AddOutput("    Usage: " + info->usage);
                        }
                    }
                }
                else
                {
                    // It's a command
                    const CommandInfo *info = console->FindCommand(arg);
                    if (info)
                    {
                        console->AddOutput(info->fullName + ": " + info->description);
                        console->AddOutput("Usage: " + info->usage);
                    }
                    else
                    {
                        console->AddOutput("Command or category not found: " + args[0]);
                    }
                }
            }
        });

    // Clear command (Source Engine style: just "clear")
    RegisterCommand("clear", "Clear console output", "clear",
                    [](const std::vector<std::string> &args, ConsoleManager *console)
                    {
                        console->ClearOutput();
                        console->AddOutput("Console cleared.");
                    });

    // Noclip command (Source Engine style)
    RegisterCommand("noclip", "Toggle player collision (noclip mode)", "noclip",
                    [](const std::vector<std::string> &args, ConsoleManager *console)
                    {
                        IPlayer *player = console->GetPlayer();
                        if (!player)
                        {
                            console->AddOutput("Error: Player instance not available.");
                            return;
                        }
                        bool current = player->IsNoclip();
                        player->SetNoclip(!current);
                        console->AddOutput("Noclip: " +
                                           std::string(!current ? "enabled" : "disabled"));
                    });

    // FPS command (Source Engine style: cl_showfps or just fps)
    RegisterCommand("m_showfps", "Show current FPS", "m_showfps",
                    [](const std::vector<std::string> &args, ConsoleManager *console)
                    {
                        int fps = GetFPS();
                        float frameTime = GetFrameTime() * 1000.0f; // Convert to milliseconds
                        console->AddOutput("FPS: " + std::to_string(fps) +
                                           " | Frame time: " + std::to_string(frameTime) + "ms");
                    });

    // Quit command (Source Engine style)
    RegisterCommand("quit", "Quit the game", "quit",
                    [](const std::vector<std::string> &args, ConsoleManager *console)
                    {
                        console->AddOutput("Quitting game...");
                        auto engine = console->GetEngine();
                        if (engine)
                        {
                            engine->RequestExit(); // TODO: Add RequestExit to Engine
                        }
                        exit(0);
                    });
}

void ConsoleManager::AddOutput(const std::string &text)
{
    consoleOutput.push_back(text);

    // Limit output lines
    if (consoleOutput.size() > MAX_CONSOLE_LINES)
    {
        consoleOutput.erase(consoleOutput.begin(),
                            consoleOutput.begin() + (consoleOutput.size() - MAX_CONSOLE_LINES));
    }
}

void ConsoleManager::ClearOutput()
{
    consoleOutput.clear();
}

void ConsoleManager::AddToHistory(const std::string &command)
{
    // Don't add duplicate consecutive commands
    if (!consoleHistory.empty() && consoleHistory.back() == command)
    {
        return;
    }

    consoleHistory.push_back(command);

    // Limit history size
    if (consoleHistory.size() > MAX_HISTORY_LINES)
    {
        consoleHistory.erase(consoleHistory.begin(),
                             consoleHistory.begin() + (consoleHistory.size() - MAX_HISTORY_LINES));
    }
}

// Scrolling functions removed as ImGui handles it

void ConsoleManager::CopyToClipboard(const std::string &text)
{
    ImGui::SetClipboardText(text.c_str());
}

void ConsoleManager::CopyLastCommand()
{
    if (!consoleHistory.empty())
    {
        CopyToClipboard(consoleHistory.back());
    }
}

std::string ConsoleManager::GetLastCommand() const
{
    if (!consoleHistory.empty())
    {
        return consoleHistory.back();
    }
    return "";
}

void ConsoleManager::RenderConsole()
{
    if (!consoleOpen)
        return;

    // Create ImGui window for console
    ImGui::SetNextWindowSize(ImVec2(800, 400), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(100, 100), ImGuiCond_FirstUseEver);
    bool open = consoleOpen;
    ImGui::Begin("Console", &open, ImGuiWindowFlags_NoCollapse);
    consoleOpen = open;

    // Toolbar with copy button
    if (ImGui::Button("Copy Last Command"))
    {
        CopyLastCommand();
    }
    ImGui::SameLine();
    if (ImGui::Button("Copy All Output"))
    {
        std::string allOutput;
        for (const auto &line : consoleOutput)
        {
            allOutput += line + "\n";
        }
        if (!allOutput.empty())
        {
            CopyToClipboard(allOutput);
        }
    }

    // Display output with context menu for copying
    ImGui::BeginChild("Output", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()), false,
                      ImGuiWindowFlags_HorizontalScrollbar);
    for (size_t i = 0; i < consoleOutput.size(); ++i)
    {
        const auto &line = consoleOutput[i];
        ImGui::TextUnformatted(line.c_str());

        // Add context menu on right-click for copying individual lines
        if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
        {
            ImGui::OpenPopup(("OutputContextMenu_" + std::to_string(i)).c_str());
        }

        if (ImGui::BeginPopup(("OutputContextMenu_" + std::to_string(i)).c_str()))
        {
            if (ImGui::MenuItem("Copy Line"))
            {
                CopyToClipboard(line);
            }
            ImGui::EndPopup();
        }
    }
    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
    {
        ImGui::SetScrollHereY(1.0f);
    }
    ImGui::EndChild();

    // History section with copy functionality
    if (!consoleHistory.empty())
    {
        ImGui::Separator();
        ImGui::Text("Command History:");
        ImGui::BeginChild("History", ImVec2(0, 100), false, ImGuiWindowFlags_HorizontalScrollbar);
        for (size_t i = 0; i < consoleHistory.size(); ++i)
        {
            const auto &cmd = consoleHistory[i];
            ImGui::Selectable(("##history_" + std::to_string(i)).c_str(), false);
            ImGui::SameLine();
            ImGui::TextUnformatted(cmd.c_str());

            // Double-click to insert command into input field
            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
            {
                // Copy to clipboard first, then user can paste
                CopyToClipboard(cmd);
            }

            // Right-click context menu for copying
            if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
            {
                ImGui::OpenPopup(("HistoryContextMenu_" + std::to_string(i)).c_str());
            }

            if (ImGui::BeginPopup(("HistoryContextMenu_" + std::to_string(i)).c_str()))
            {
                if (ImGui::MenuItem("Copy Command"))
                {
                    CopyToClipboard(cmd);
                }
                ImGui::EndPopup();
            }
        }
        ImGui::EndChild();
    }

    // Input area
    ImGui::Separator();
    static char inputBuffer[256] = "";
    ImGui::PushItemWidth(-1);
    if (ImGui::InputText("##Input", inputBuffer, IM_ARRAYSIZE(inputBuffer),
                         ImGuiInputTextFlags_EnterReturnsTrue))
    {
        std::string command = inputBuffer;
        if (!command.empty())
        {
            ExecuteCommand(command);
            AddToHistory(command);
            inputBuffer[0] = '\0';
        }
    }
    ImGui::PopItemWidth();

    ImGui::End();
}
const std::vector<std::string> &ConsoleManager::GetOutput() const
{
    return consoleOutput;
}
const std::vector<std::string> &ConsoleManager::GetHistory() const
{
    return consoleHistory;
}
bool ConsoleManager::IsConsoleOpen() const
{
    return consoleOpen;
}


