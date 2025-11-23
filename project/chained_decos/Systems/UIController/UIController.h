#ifndef UI_CONTROLLER_H
#define UI_CONTROLLER_H

#include "core/object/module/Interfaces/IEngineModule.h"
#include "core/object/kernel/Core/Kernel.h"
#include "project/chained_decos/Menu/Menu.h"
#include "project/chained_decos/Menu/Console/ConsoleManager.h"
#include "Engine/Interfaces/IEngine.h"
#include <memory>
#include <vector>
#include <string>

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

    // Menu action handling
    void HandleMenuActions(bool* showMenu, bool* isGameInitialized);

private:
    // System OWNS its components
    std::unique_ptr<Menu> m_menu;
    
    // Kernel reference (for accessing services)
    Kernel* m_kernel;
    
    // Dependencies obtained through Kernel (references only)
    Engine* m_engine;

    // Individual action handlers
    void HandleSinglePlayer(bool* showMenu, bool* isGameInitialized);
    void HandleResumeGame(bool* showMenu, bool* isGameInitialized);
    void HandleStartGameWithMap(bool* showMenu, bool* isGameInitialized);
    void HandleExitGame(bool* showMenu);

    // Helper methods
    void HideMenuAndStartGame(bool* showMenu);
    void EnsurePlayerSafePosition();
    void ReinitializeCollisionSystemForResume();
    std::string ConvertMapNameToPath(const std::string& selectedMapName);
    std::vector<std::string> AnalyzeMapForRequiredModels(const std::string& mapPath);
    bool LoadRequiredModels(const std::vector<std::string>& requiredModels);
    bool InitializeCollisionSystemWithModels(const std::vector<std::string>& requiredModels);
    void LoadMapObjects(const std::string& mapPath);
    void RegisterPreloadedModels();
    void CreateModelInstancesForMap();
    bool AutoLoadModelIfNeeded(const std::string& requested, std::string& candidateName);
};

#endif // UI_CONTROLLER_H

