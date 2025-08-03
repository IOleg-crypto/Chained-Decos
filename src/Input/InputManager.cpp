//
// Created by I#Oleg.
//

#include "InputManager.h"

void InputManager::RegisterAction(const int key, const std::function<void()> &action) {
    m_actions[key] = action;
}

void InputManager::ProcessInput() const {
    for (const auto& [key, action] : m_actions) {
        if (IsKeyPressed(key)) {
            action();
        }
    }
}

