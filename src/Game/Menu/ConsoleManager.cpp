#include "ConsoleManager.h"
#include "../Game.h"
#include "Game/Player/PlayerCollision.h"
#include <raylib.h>
#include <imgui/imgui.h>
#include <iostream>
#include <algorithm>
#include <sstream>

ConsoleManager::ConsoleManager(Game* game) : m_game(game) {
    TraceLog(LOG_INFO, "ConsoleManager::ConsoleManager() - CONSOLE MANAGER BEING INITIALIZED");

    // Note: Font loading removed as it's not needed for ImGui integration

    TraceLog(LOG_INFO, "ConsoleManager::ConsoleManager() - CONSOLE MANAGER INITIALIZED");
}

void ConsoleManager::ToggleConsole() {
    consoleOpen = !consoleOpen;
    if (consoleOpen) {
        OpenConsole();
    } else {
        CloseConsole();
    }
}

void ConsoleManager::OpenConsole() {
    consoleOpen = true;
}

void ConsoleManager::CloseConsole() {
    consoleOpen = false;
}

// Input handling removed as ImGui handles it

void ConsoleManager::ExecuteCommand(const std::string& command) {
    AddOutput("> " + command);

    std::istringstream iss(command);
    std::string cmd;
    iss >> cmd;

    std::string args;
    std::getline(iss, args);

    // Remove leading whitespace from args
    args.erase(0, args.find_first_not_of(" \t"));

    if (cmd == "help") {
        ProcessHelpCommand();
    } else if (cmd == "clear") {
        ProcessClearCommand();
    } else if (cmd == "set") {
        std::istringstream argStream(args);
        std::string key, value;
        argStream >> key >> value;
        ProcessSetCommand(key, value);
    } else if (cmd == "get") {
        ProcessGetCommand(args);
    } else if (cmd == "noclip") {
        ProcessNoclipCommand();
    } else if (cmd == "speed") {
        std::istringstream argStream(args);
        float speed;
        if (argStream >> speed) {
            if (!m_game) {
                AddOutput("Error: Game instance not available.");
                return;
            }
            Player& player = m_game->GetPlayer();
            player.GetMovement()->SetSpeed(speed);
            AddOutput("Player speed set to " + std::to_string(speed));
        } else {
            AddOutput("Usage: speed <value> - <value> must be a number.");
        }
    } else if (cmd.empty()) {
        // Empty command, do nothing
    } else {
        AddOutput("Unknown command: " + cmd + ". Type 'help' for available commands.");
    }
}

void ConsoleManager::ProcessHelpCommand() {
    AddOutput("Available commands:");
    auto commands = GetAvailableCommands();
    for (const auto& cmd : commands) {
        AddOutput("  " + cmd);
    }
}

void ConsoleManager::ProcessClearCommand() {
    ClearOutput();
    AddOutput("Console cleared.");
}

void ConsoleManager::ProcessSetCommand(const std::string& key, const std::string& value) {
    if (key.empty() || value.empty()) {
        AddOutput("Usage: set <variable> <value>");
        return;
    }

    // Here you would integrate with the settings manager to actually set values
    AddOutput("Setting " + key + " to " + value + " (not implemented yet)");
}

void ConsoleManager::ProcessGetCommand(const std::string& key) {
    if (key.empty()) {
        AddOutput("Usage: get <variable>");
        return;
    }

    // Here you would integrate with the settings manager to get values
    AddOutput("Getting " + key + " (not implemented yet)");
}

void ConsoleManager::ProcessNoclipCommand() {
    if (!m_game) {
        AddOutput("Error: Game instance not available.");
        return;
    }

    Player& player = m_game->GetPlayer();
    PlayerCollision& collision = player.GetCollisionMutable();
    bool current = collision.IsUsingBVH();
    collision.EnableBVHCollision(!current);
    AddOutput("Noclip toggled: " + std::string(!current ? "enabled" : "disabled"));
}

std::vector<std::string> ConsoleManager::GetAvailableCommands() const {
    return {
        "help - Show available commands",
        "clear - Clear console output",
        "set <var> <value> - Set a variable",
        "get <var> - Get a variable value",
        "noclip - Toggle player collision",
        "speed <value> - Set player speed"
    };
}

void ConsoleManager::AddOutput(const std::string& text) {
    consoleOutput.push_back(text);

    // Limit output lines
    if (consoleOutput.size() > MAX_CONSOLE_LINES) {
        consoleOutput.erase(consoleOutput.begin(), consoleOutput.begin() + (consoleOutput.size() - MAX_CONSOLE_LINES));
    }
}

void ConsoleManager::ClearOutput() {
    consoleOutput.clear();
}

void ConsoleManager::AddToHistory(const std::string& command) {
    // Don't add duplicate consecutive commands
    if (!consoleHistory.empty() && consoleHistory.back() == command) {
        return;
    }

    consoleHistory.push_back(command);

    // Limit history size
    if (consoleHistory.size() > MAX_HISTORY_LINES) {
        consoleHistory.erase(consoleHistory.begin(), consoleHistory.begin() + (consoleHistory.size() - MAX_HISTORY_LINES));
    }
}

// Scrolling functions removed as ImGui handles scrolling

void ConsoleManager::CopyToClipboard(const std::string& text) {
    ImGui::SetClipboardText(text.c_str());
}

void ConsoleManager::CopyLastCommand() {
    if (!consoleHistory.empty()) {
        CopyToClipboard(consoleHistory.back());
    }
}

std::string ConsoleManager::GetLastCommand() const {
    if (!consoleHistory.empty()) {
        return consoleHistory.back();
    }
    return "";
}

void ConsoleManager::RenderConsole() {
    if (!consoleOpen) return;

    // Create ImGui window for console
    ImGui::SetNextWindowSize(ImVec2(800, 400), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(100, 100), ImGuiCond_FirstUseEver);
    bool open = consoleOpen;
    ImGui::Begin("Console", &open, ImGuiWindowFlags_NoCollapse);
    consoleOpen = open;

    // Toolbar with copy button
    if (ImGui::Button("Copy Last Command")) {
        CopyLastCommand();
    }
    ImGui::SameLine();
    if (ImGui::Button("Copy All Output")) {
        std::string allOutput;
        for (const auto& line : consoleOutput) {
            allOutput += line + "\n";
        }
        if (!allOutput.empty()) {
            CopyToClipboard(allOutput);
        }
    }

    // Display output with context menu for copying
    ImGui::BeginChild("Output", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()), false, ImGuiWindowFlags_HorizontalScrollbar);
    for (size_t i = 0; i < consoleOutput.size(); ++i) {
        const auto& line = consoleOutput[i];
        ImGui::TextUnformatted(line.c_str());
        
        // Add context menu on right-click for copying individual lines
        if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
            ImGui::OpenPopup(("OutputContextMenu_" + std::to_string(i)).c_str());
        }
        
        if (ImGui::BeginPopup(("OutputContextMenu_" + std::to_string(i)).c_str())) {
            if (ImGui::MenuItem("Copy Line")) {
                CopyToClipboard(line);
            }
            ImGui::EndPopup();
        }
    }
    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
        ImGui::SetScrollHereY(1.0f);
    }
    ImGui::EndChild();

    // History section with copy functionality
    if (!consoleHistory.empty()) {
        ImGui::Separator();
        ImGui::Text("Command History:");
        ImGui::BeginChild("History", ImVec2(0, 100), false, ImGuiWindowFlags_HorizontalScrollbar);
        for (size_t i = 0; i < consoleHistory.size(); ++i) {
            const auto& cmd = consoleHistory[i];
            ImGui::Selectable(("##history_" + std::to_string(i)).c_str(), false);
            ImGui::SameLine();
            ImGui::TextUnformatted(cmd.c_str());
            
            // Double-click to insert command into input field
            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                // Copy to clipboard first, then user can paste
                CopyToClipboard(cmd);
            }
            
            // Right-click context menu for copying
            if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
                ImGui::OpenPopup(("HistoryContextMenu_" + std::to_string(i)).c_str());
            }
            
            if (ImGui::BeginPopup(("HistoryContextMenu_" + std::to_string(i)).c_str())) {
                if (ImGui::MenuItem("Copy Command")) {
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
    if (ImGui::InputText("##Input", inputBuffer, IM_ARRAYSIZE(inputBuffer), ImGuiInputTextFlags_EnterReturnsTrue)) {
        std::string command = inputBuffer;
        if (!command.empty()) {
            ExecuteCommand(command);
            AddToHistory(command);
            inputBuffer[0] = '\0';
        }
    }
    ImGui::PopItemWidth();

    ImGui::End();
}