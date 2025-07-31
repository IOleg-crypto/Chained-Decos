//
// Created by I#Oleg.
//

#ifndef INPUTMANAGER_H
#define INPUTMANAGER_H

#include <functional>
#include <unordered_map>
#include <raylib.h>

class InputManager {
private:
    std::unordered_map<int , std::function<void()>>m_action;
public:
    InputManager() = default;
    ~InputManager() = default;
    InputManager(const InputManager& other) = delete;
    InputManager(InputManager&& other) = delete;
public:
    // Add input key to manager
    void RegisterAction(int key , const std::function<void()> &action);
    // Access to input key
    void ProcessInput() const;
};




#endif //INPUTMANAGER_H
