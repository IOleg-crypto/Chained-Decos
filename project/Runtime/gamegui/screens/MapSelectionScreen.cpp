#include "MapSelectionScreen.h"
#include "../mapselector/mapselector.h"
#include "../Menu.h"
#include <imgui.h>
#include <raylib.h>

void MapSelectionScreen::Update()
{
    // Logic from HandleKeyboardNavigation for MapSelection state
    auto mapSelector = GetMenu()->GetMapSelector();
    if (mapSelector && mapSelector->HasMaps())
    {
        mapSelector->HandleKeyboardNavigation();
        if (IsKeyPressed(KEY_ENTER))
        {
            GetMenu()->DispatchEvent(CHEngine::MenuEventType::StartGameWithMap,
                                     GetMenu()->GetSelectedMapName());
        }
    }
}

void MapSelectionScreen::Render()
{
    if (!GetMenu())
        return;

    auto mapSelector = GetMenu()->GetMapSelector();
    if (mapSelector)
    {
        MapSelector::InteractionResult result = mapSelector->RenderMapSelectionImGui();

        if (result == MapSelector::InteractionResult::LoadMap)
        {
            GetMenu()->DispatchEvent(CHEngine::MenuEventType::StartGameWithMap,
                                     GetMenu()->GetSelectedMapName());
        }
        else if (result == MapSelector::InteractionResult::Back)
        {
            GetMenu()->ShowMainMenu();
        }
    }
    else
    {
        const ImVec2 windowSize = ImGui::GetIO().DisplaySize;
        ImGui::SetCursorPos(ImVec2((windowSize.x - 200) * 0.5f, windowSize.y * 0.5f));
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "No maps available");

        ImGui::SetCursorPos(ImVec2(50, windowSize.y - 85.0f));
        if (ImGui::Button("< BACK", ImVec2(100, 40)))
        {
            GetMenu()->ShowMainMenu();
        }
    }
}

