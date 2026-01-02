#include "gui_manager.h"
#include "rlImGui.h"

namespace CHEngine
{
static std::unique_ptr<GuiManager> s_Instance = nullptr;

void GuiManager::Init()
{
    s_Instance = std::unique_ptr<GuiManager>(new GuiManager());
}

bool GuiManager::IsInitialized()
{
    return s_Instance != nullptr;
}

void GuiManager::Shutdown()
{
    if (s_Instance)
    {
        s_Instance->InternalShutdown();
        s_Instance.reset();
    }
}

void GuiManager::Update(float deltaTime)
{
    s_Instance->InternalUpdate(deltaTime);
}

void GuiManager::Render()
{
    s_Instance->InternalRender();
}

void GuiManager::AddElement(std::shared_ptr<GuiElement> element)
{
    s_Instance->InternalAddElement(element);
}

void GuiManager::RemoveElement(std::shared_ptr<GuiElement> element)
{
    s_Instance->InternalRemoveElement(element);
}

void GuiManager::Clear()
{
    s_Instance->InternalClear();
}

bool GuiManager::IsVisible()
{
    return s_Instance->InternalIsVisible();
}

void GuiManager::SetVisible(bool visible)
{
    s_Instance->InternalSetVisible(visible);
}

GuiManager::GuiManager() : m_visible(true)
{
    rlImGuiSetup(true);
}

GuiManager::~GuiManager()
{
    InternalShutdown();
}

void GuiManager::InternalUpdate(float deltaTime)
{
    if (!m_visible)
        return;

    for (auto &element : m_elements)
    {
        if (element->IsEnabled())
        {
            element->Update(deltaTime);
            element->HandleInput();
        }
    }
}

void GuiManager::InternalRender()
{
    if (!m_visible)
        return;

    for (auto &element : m_elements)
    {
        if (element->IsVisible())
        {
            element->Render();
        }
    }
}

void GuiManager::InternalShutdown()
{
    InternalClear();
    rlImGuiShutdown();
}
} // namespace CHEngine
