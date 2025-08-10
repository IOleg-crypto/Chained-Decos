//
// Created by I#Oleg.
//

#ifndef INPUTMANAGER_H
#define INPUTMANAGER_H

#include <functional>
#include <unordered_map>
#include <raylib.h>

//
// InputManager
// Manages input actions mapped to keyboard keys.
// Allows registration of callbacks that get called when keys are processed.
//
class InputManager {
public:
    InputManager() = default;
    ~InputManager() = default;

    InputManager(const InputManager& other) = delete;
    InputManager(InputManager&& other) = delete;

    // Registers an action callback to a specific key
    // key    - The keyboard key to listen for (e.g. KEY_W)
    // action - The function to call when key is pressed
    void RegisterAction(int key, const std::function<void()>& action);

    // Processes input, calling registered callbacks if their keys are pressed
    void ProcessInput() const;

private:
    std::unordered_map<int, std::function<void()>> m_actions; // Map of key to action callback
};

#endif // INPUTMANAGER_H
