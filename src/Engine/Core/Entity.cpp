//
// Created by olegg on 07.09.2025.
//

#include "Entity.h"

template<typename T, typename ... Args>
T * Entity::AddComponent(Args &&...args) {
    auto component = std::make_unique<T>(std::forward<Args>(args)...);
    component->SetOwner(this);
    T* componentPtr = component.get();
    m_components[typeid(T)] = std::move(component);
    return componentPtr;
}

template<typename T>
T * Entity::GetComponent() const {
    auto it = m_components.find(typeid(T));
    return it != m_components.end() ? dynamic_cast<T*>(it->second.get()) : nullptr;
}

template<typename T>
void Entity::RemoveComponent() {
    m_components.erase(typeid(T));
}

template<typename T>
bool Entity::HasComponent() const {
    return m_components.find(typeid(T)) != m_components.end();
}

void Entity::Start() {
    for (auto& [type, component] : m_components) {
        component->Start();
    }
}

void Entity::Update(float deltaTime) {
    for (auto& [type, component] : m_components) {
        component->Update(deltaTime);
    }
}

const std::string & Entity::GetName() const { return m_name; }

void Entity::SetName(const std::string &name) { m_name = name; }
