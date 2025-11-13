#ifndef UI_CONTROLLER_H
#define UI_CONTROLLER_H

#include "Engine/Module/Interfaces/IEngineModule.h"
#include "Engine/Kernel/Core/Kernel.h"
#include <memory>
#include <vector>
#include <string>

class Menu;
class ConsoleManager;
class Engine;

// System for managing user interface and menus
// Creates and owns its components independently
class UIController : public IEngineModule {
public:
    UIController();
    ~UIController() override;

    // IEngineModule interface
    const char* GetModuleName() const override { return "UI"; }
    const char* GetModuleVersion() const override { return "1.0.0"; }
    const char* GetModuleDescription() const override { 
        return "User interface and menu management"; 
    }
    
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
    // System OWNS its components
    std::unique_ptr<Menu> m_menu;
    
    // Kernel reference (for accessing services)
    Kernel* m_kernel;
    
    // Dependencies obtained through Kernel (references only)
    Engine* m_engine;
};

#endif // UI_CONTROLLER_H

