#ifndef CH_PROJECT_SELECTOR_UI_H
#define CH_PROJECT_SELECTOR_UI_H

#include "editor/editor_project_manager.h"
#include <cstdint>

namespace CHEngine
{

class ProjectSelectorUI
{
public:
    ProjectSelectorUI(EditorProjectManager& projectManager);

    void OnImGuiRender();

private:
    EditorProjectManager& m_ProjectManager;
    uint64_t m_NewProjectIconHandle = 0;
    uint64_t m_OpenProjectIconHandle = 0;
};

} // namespace CHEngine

#endif // CH_PROJECT_SELECTOR_UI_H
