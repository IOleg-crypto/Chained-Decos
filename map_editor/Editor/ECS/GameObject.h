#pragma once

#include "Component.h"
#include <algorithm>
#include <memory>
#include <string>
#include <typeindex>
#include <vector>


class GameObject
{
public:
    GameObject(const std::string &name = "GameObject");

    virtual ~GameObject() = default;

    void Start();
    void Update(float deltaTime);
    void Render();

    // Template methods for component management
    template <typename T, typename... Args> T *AddComponent(Args &&...args)
    {
        auto component = std::make_unique<T>(this, std::forward<Args>(args)...);
        T *rawPtr = component.get();
        m_components.emplace_back(std::move(component));
        return rawPtr;
    }

    template <typename T> T *GetComponent()
    {
        for (auto &component : m_components)
        {
            if (auto casted = dynamic_cast<T *>(component.get()))
            {
                return casted;
            }
        }
        return nullptr;
    }

    // Name and Active state
    const std::string &GetName() const;
    void SetName(const std::string &name);

    bool IsActive() const;
    void SetActive(bool active);

    // Hierarchy
    GameObject *GetParent() const;
    void SetParent(GameObject *parent);

    const std::vector<std::unique_ptr<Component>> &GetComponents() const;

private:
    std::string m_name;
    bool m_active;
    GameObject *m_parent;
    std::vector<std::unique_ptr<Component>> m_components;
};
