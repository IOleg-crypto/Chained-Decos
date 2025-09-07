#pragma once

#include <string>

class Entity;

class Component {
public:
    virtual ~Component() = default;

    // Core lifecycle methods
    virtual void Start() {}
    virtual void Update(float deltaTime) {}

    // Owner management
    void SetOwner(Entity* owner);

    [[nodiscard]] Entity* GetOwner() const;

    // Enable/Disable
    bool IsEnabled() const { return m_enabled; }
    void SetEnabled(bool enabled);

protected:
    Entity* m_owner = nullptr;
    bool m_enabled = true;
};


