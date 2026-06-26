#ifndef CH_PROJECT_SELECTOR_UI_H
#define CH_PROJECT_SELECTOR_UI_H

#include "editor/editor_project_manager.h"
#include <cstdint>
#include <memory>


namespace Chained
{
class TextureAsset; // Вперед-оголошення

class ProjectSelectorUI
{
public:
    ProjectSelectorUI(EditorProjectManager& projectManager);

    void OnImGuiRender();

private:
    EditorProjectManager& m_ProjectManager;

    // Зберігаємо вказівники на текстури, щоб не залежати від геймплейних UUID проєкту
    std::shared_ptr<TextureAsset> m_NewProjectIcon = nullptr;
    std::shared_ptr<TextureAsset> m_OpenProjectIcon = nullptr;
    bool m_IconsLoaded = false;

    void LoadEditorIcons(); // Метод для безпечного завантаження іконок UI
};

} // namespace Chained

#endif // CH_PROJECT_SELECTOR_UI_H