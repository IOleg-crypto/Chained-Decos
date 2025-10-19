#include "ConsoleManager.h"
#include <raylib.h>
#include <iostream>
#include <algorithm>
#include <sstream>

ConsoleManager::ConsoleManager() {
    TraceLog(LOG_INFO, "ConsoleManager::ConsoleManager() - CONSOLE MANAGER BEING INITIALIZED");

    // Load font for console (use default if Alan Sans is not available)
    consoleFont = GetFontDefault();

    // Try to load Alan Sans font for better console appearance
    const std::string alanSansFontPath = PROJECT_ROOT_DIR "/resources/font/AlanSans.ttf";
    consoleFont = LoadFontEx(alanSansFontPath.c_str(), 20, nullptr, 0);

    if (consoleFont.texture.id == 0) {
        TraceLog(LOG_WARNING, "ConsoleManager::ConsoleManager() - Failed to load Alan Sans font for console, using default font");
        consoleFont = GetFontDefault();
    } else {
        SetTextureFilter(consoleFont.texture, TEXTURE_FILTER_BILINEAR);
        TraceLog(LOG_INFO, "ConsoleManager::ConsoleManager() - Alan Sans font loaded successfully");
    }

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
    consoleInput.clear();
    consoleHistoryIndex = consoleHistory.size();
    scrollOffset = 0;
}

void ConsoleManager::CloseConsole() {
    consoleOpen = false;
    consoleInput.clear();
    scrollOffset = 0;
}

void ConsoleManager::HandleInput() {
    if (!consoleOpen) return;

    // Handle text input
    int key = GetCharPressed();
    while (key > 0) {
        ProcessInputCharacter(static_cast<char>(key));
        key = GetCharPressed();
    }

    // Handle backspace
    if (IsKeyPressed(KEY_BACKSPACE)) {
        ProcessBackspace();
    }

    // Handle enter
    if (IsKeyPressed(KEY_ENTER)) {
        ProcessEnter();
    }

    // Handle history navigation
    if (IsKeyPressed(KEY_UP)) {
        NavigateHistory(true);
    }
    if (IsKeyPressed(KEY_DOWN)) {
        NavigateHistory(false);
    }

    // Handle scrolling
    if (IsKeyPressed(KEY_PAGE_UP)) {
        ScrollUp();
    }
    if (IsKeyPressed(KEY_PAGE_DOWN)) {
        ScrollDown();
    }
}

void ConsoleManager::ProcessInputCharacter(char character) {
    if (character >= 32 && character <= 125) { // Printable characters
        consoleInput += character;
    }
}

void ConsoleManager::ProcessBackspace() {
    if (!consoleInput.empty()) {
        consoleInput.pop_back();
    }
}

void ConsoleManager::ProcessEnter() {
    if (!consoleInput.empty()) {
        ExecuteCommand(consoleInput);
        AddToHistory(consoleInput);
        consoleInput.clear();
        consoleHistoryIndex = consoleHistory.size();
    }
}

void ConsoleManager::NavigateHistory(bool up) {
    if (consoleHistory.empty()) return;

    if (up && consoleHistoryIndex > 0) {
        consoleHistoryIndex--;
        consoleInput = consoleHistory[consoleHistoryIndex];
    } else if (!up && consoleHistoryIndex < consoleHistory.size() - 1) {
        consoleHistoryIndex++;
        consoleInput = consoleHistory[consoleHistoryIndex];
    } else if (!up && consoleHistoryIndex == consoleHistory.size() - 1) {
        consoleHistoryIndex = consoleHistory.size();
        consoleInput.clear();
    }
}

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

std::vector<std::string> ConsoleManager::GetAvailableCommands() const {
    return {
        "help - Show available commands",
        "clear - Clear console output",
        "set <var> <value> - Set a variable",
        "get <var> - Get a variable value"
    };
}

void ConsoleManager::AddOutput(const std::string& text) {
    consoleOutput.push_back(text);

    // Limit output lines
    if (consoleOutput.size() > MAX_CONSOLE_LINES) {
        consoleOutput.erase(consoleOutput.begin(), consoleOutput.begin() + (consoleOutput.size() - MAX_CONSOLE_LINES));
    }

    // Auto-scroll to bottom when new output is added
    scrollOffset = 0;
}

void ConsoleManager::ClearOutput() {
    consoleOutput.clear();
    scrollOffset = 0;
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

int ConsoleManager::GetVisibleLineCount() const {
    int availableHeight = consoleHeight - inputHeight - 10; // 10px padding
    return availableHeight / lineHeight;
}

void ConsoleManager::ScrollUp() {
    int maxScroll = std::max(0, static_cast<int>(consoleOutput.size()) - GetVisibleLineCount());
    scrollOffset = std::min(maxScroll, scrollOffset + 1);
}

void ConsoleManager::ScrollDown() {
    scrollOffset = std::max(0, scrollOffset - 1);
}

void ConsoleManager::RenderConsole() const {
    if (!consoleOpen) return;

    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();

    // Draw console background
    DrawRectangle(0, 0, screenWidth, consoleHeight, Fade(BLACK, 0.8f));
    DrawRectangleLines(0, 0, screenWidth, consoleHeight, WHITE);

    // Draw console title
    DrawText("Console (F1: Help, ~: Close)", 10, 10, 20, WHITE);

    // Calculate visible area
    int outputStartY = 30;
    int outputHeight = consoleHeight - inputHeight - 40;
    int maxVisibleLines = outputHeight / lineHeight;

    // Draw output lines
    int startLine = std::max(0, static_cast<int>(consoleOutput.size()) - maxVisibleLines - scrollOffset);
    int endLine = std::min(static_cast<int>(consoleOutput.size()), startLine + maxVisibleLines);

    for (int i = startLine; i < endLine; ++i) {
        int y = outputStartY + (i - startLine) * lineHeight;
        DrawText(consoleOutput[i].c_str(), 10, y, 16, WHITE);
    }

    // Draw input area
    int inputY = consoleHeight - inputHeight - 5;
    DrawRectangle(5, inputY, screenWidth - 10, inputHeight, Fade(DARKGRAY, 0.5f));
    DrawRectangleLines(5, inputY, screenWidth - 10, inputHeight, WHITE);

    // Draw input text
    std::string displayInput = "> " + consoleInput;
    if (static_cast<int>(GetTime() * 2) % 2) { // Blinking cursor
        displayInput += "_";
    }
    DrawText(displayInput.c_str(), 15, inputY + 5, 18, WHITE);

    // Draw scroll indicator if needed
    if (static_cast<int>(consoleOutput.size()) > maxVisibleLines) {
        float scrollPercent = static_cast<float>(scrollOffset) / (consoleOutput.size() - maxVisibleLines);
        int scrollbarHeight = outputHeight;
        int scrollbarY = outputStartY;

        // Draw scrollbar background
        DrawRectangle(screenWidth - 15, scrollbarY, 10, scrollbarHeight, Fade(DARKGRAY, 0.5f));

        // Draw scrollbar thumb
        int thumbHeight = std::max(20, static_cast<int>(scrollbarHeight * (static_cast<float>(maxVisibleLines) / consoleOutput.size())));
        int thumbY = scrollbarY + static_cast<int>(scrollPercent * (scrollbarHeight - thumbHeight));
        DrawRectangle(screenWidth - 15, thumbY, 10, thumbHeight, WHITE);
    }
}