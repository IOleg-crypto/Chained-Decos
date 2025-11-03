#ifndef EDITOR_APPLICATION_H
#define EDITOR_APPLICATION_H

#include "Engine/Application/EngineApplication.h"
#include <memory>

// Forward declaration
class Editor;

// Editor application - використовує повний рушій + власні модулі
class EditorApplication : public EngineApplication {
public:
    EditorApplication();
    ~EditorApplication();

protected:
    // Налаштування перед ініціалізацією
    void OnPreInitialize() override;
    
    // Ініціалізація компонентів Editor
    void OnInitializeServices() override;
    
    // Реєстрація модулів Editor (ОБОВ'ЯЗКОВО)
    void OnRegisterProjectModules() override;
    
    // Реєстрація сервісів Editor
    void OnRegisterProjectServices() override;
    
    // Після ініціалізації
    void OnPostInitialize() override;
    
    // Кастомна логіка оновлення
    void OnPostUpdate(float deltaTime) override;
    
    // Кастомна логіка рендерингу
    void OnPostRender() override;
    
    // Перед завершенням
    void OnPreShutdown() override;

private:
    std::unique_ptr<Editor> m_editor;
};

#endif // EDITOR_APPLICATION_H

