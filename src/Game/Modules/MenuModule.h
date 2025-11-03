#ifndef MENU_MODULE_H
#define MENU_MODULE_H

#include "Engine/Module/IEngineModule.h"
#include <memory>

// Forward declarations
class Kernel;
class Menu;
class ConsoleManager;
class Engine;

// Модуль для управління меню та консоллю
class MenuModule : public IEngineModule {
public:
    MenuModule();
    ~MenuModule() override = default;

    // IEngineModule interface
    const char* GetModuleName() const override { return "Menu"; }
    const char* GetModuleVersion() const override { return "1.0.0"; }
    const char* GetModuleDescription() const override { return "Menu and console management"; }
    
    bool Initialize(Kernel* kernel) override;
    void Shutdown() override;
    
    void Update(float deltaTime) override;
    void Render() override;
    
    void RegisterServices(Kernel* kernel) override;
    std::vector<std::string> GetDependencies() const override;

    // Accessors
    Menu* GetMenu() const { return m_menu.get(); }
    ConsoleManager* GetConsoleManager() const;

private:
    std::unique_ptr<Menu> m_menu;
    
    // Dependencies
    Engine* m_engine;
};

#endif // MENU_MODULE_H

