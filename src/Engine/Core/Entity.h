#ifndef ENTITY_H
#define ENTITY_H

#include <memory>
#include <string>
#include <unordered_map>
#include <typeindex>
#include "Component.h"


class Entity {
public:
    explicit Entity(std::string name = "Entity") : m_name(std::move(name)) {}
    virtual ~Entity() = default;

    // Add a component
    template<typename T, typename... Args>
    T* AddComponent(Args&&... args);

    // Get a component
    template<typename T>
    T* GetComponent() const;

    // Remove a component
    template<typename T>
    void RemoveComponent();

    // Check if has component
    template<typename T>
    bool HasComponent() const;

    // Lifecycle methods
    virtual void Start();

    virtual void Update(float deltaTime);

    // Getters/Setters
    [[nodiscard]] const std::string& GetName() const;

    void SetName(const std::string& name);

private:
    std::string m_name;
    std::unordered_map<std::type_index, std::unique_ptr<Component>> m_components;
};



#endif
