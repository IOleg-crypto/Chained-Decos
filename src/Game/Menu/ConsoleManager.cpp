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

void ConsoleManager::RenderConsole() {
    if (!consoleOpen) return;

    // Create ImGui window for console
    ImGui::SetNextWindowSize(ImVec2(800, 400), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(100, 100), ImGuiCond_FirstUseEver);
    bool open = consoleOpen;
    ImGui::Begin("Console", &open, ImGuiWindowFlags_NoCollapse);
    consoleOpen = open;

    // Display output
    ImGui::BeginChild("Output", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()), false, ImGuiWindowFlags_HorizontalScrollbar);
    for (const auto& line : consoleOutput) {
        ImGui::TextUnformatted(line.c_str());
    }
    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
        ImGui::SetScrollHereY(1.0f);
    }
    ImGui::EndChild();

    // Input area
    ImGui::Separator();
    static char inputBuffer[256] = "";
    if (ImGui::InputText("##Input", inputBuffer, IM_ARRAYSIZE(inputBuffer), ImGuiInputTextFlags_EnterReturnsTrue)) {
        std::string command = inputBuffer;
        if (!command.empty()) {
            ExecuteCommand(command);
            AddToHistory(command);
            inputBuffer[0] = '\0';
        }
    }

    ImGui::End();
}