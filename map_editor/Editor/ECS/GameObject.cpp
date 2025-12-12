#include "GameObject.h"

GameObject::GameObject(const std::string &name) : m_name(name), m_active(true), m_parent(nullptr)
{
}

void GameObject::Start()
{
    for (auto &component : m_components)
    {
        if (component->IsEnabled())
            component->OnStart();
    }
}

void GameObject::Update(float deltaTime)
{
    if (!m_active)
        return;
    for (auto &component : m_components)
    {
        if (component->IsEnabled())
            component->OnUpdate(deltaTime);
    }
}

void GameObject::Render()
{
    if (!m_active)
        return;
    for (auto &component : m_components)
    {
        if (component->IsEnabled())
            component->OnRender();
    }
}

const std::string &GameObject::GetName() const
{
    return m_name;
}

void GameObject::SetName(const std::string &name)
{
    m_name = name;
}

bool GameObject::IsActive() const
{
    return m_active;
}

void GameObject::SetActive(bool active)
{
    m_active = active;
}

GameObject *GameObject::GetParent() const
{
    return m_parent;
}

void GameObject::SetParent(GameObject *parent)
{
    m_parent = parent;
}

const std::vector<std::unique_ptr<Component>> &GameObject::GetComponents() const
{
    return m_components;
}
