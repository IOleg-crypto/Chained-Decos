#include "MenuModule.h"
#include "Engine/Kernel/Kernel.h"
#include "Engine/Engine.h"
#include "Game/Menu/Menu.h"
#include "Game/Menu/ConsoleManager.h"
#include <raylib.h>

MenuModule::MenuModule()
    : m_engine(nullptr)
{
}

bool MenuModule::Initialize(Kernel* kernel)
{
    if (!kernel) {
        TraceLog(LOG_ERROR, "[MenuModule] Kernel is null");
        return false;
    }

    // Отримуємо Engine з Kernel (якщо зареєстрований)
    // Для Menu потрібен Engine для ініціалізації
    // Якщо Engine не зареєстрований як сервіс, це нормально для цього модуля
    
    // Створюємо Menu
    m_menu = std::make_unique<Menu>();

    TraceLog(LOG_INFO, "[MenuModule] Initialized successfully");
    return true;
}

void MenuModule::Shutdown()
{
    if (m_menu) {
        m_menu.reset();
    }
    
    m_engine = nullptr;
    
    TraceLog(LOG_INFO, "[MenuModule] Shutdown complete");
}

void MenuModule::Update(float deltaTime)
{
    if (m_menu) {
        m_menu->Update();
    }
}

void MenuModule::Render()
{
    // Menu rendering handled separately
}

void MenuModule::RegisterServices(Kernel* kernel)
{
    if (!kernel) return;
    
    // Menu може бути зареєстрований як сервіс, але зараз це не потрібно
}

std::vector<std::string> MenuModule::GetDependencies() const
{
    return {}; // No dependencies
}

ConsoleManager* MenuModule::GetConsoleManager() const
{
    if (!m_menu) return nullptr;
    return m_menu->GetConsoleManager();
}

