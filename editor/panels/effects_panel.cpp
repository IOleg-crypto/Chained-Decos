#include "effects_panel.h"
#include "editor/editor_layer.h"
#include "engine/runtime/application.h"
#include "engine/graphics/pipeline/renderer.h"
#include "scene/scene.h"
#include "engine/core/service_locator.h"


namespace Chained
{

EffectsPanel::EffectsPanel()
{
    m_Name = "Effects & Debug";
}

void EffectsPanel::OnImGuiRender(bool readOnly)
{
    if (!m_IsOpen)
    {
        return;
    }

    ImGui::Begin(m_Name.c_str(), &m_IsOpen);

    if (!m_Context)
    {
        ImGui::Text("No active scene.");
        ImGui::End();
        return;
    }

    if (ImGui::CollapsingHeader("Viewport Settings", ImGuiTreeNodeFlags_DefaultOpen))
    {
        const char* diagnosticModes[] = {"Full Render", "Normals", "Lighting only", "Albedo only"};
        int currentDiag = (int)m_Context->GetSettings().DiagnosticMode;
        if (ImGui::Combo("Diagnostic Mode", &currentDiag, diagnosticModes, 4))
        {
            m_Context->GetSettings().DiagnosticMode = (float)currentDiag;
            ServiceLocator::Get<Renderer>()->SetDiagnosticMode((float)currentDiag);
        }
    }

    if (ImGui::CollapsingHeader("Debug Visualization", ImGuiTreeNodeFlags_DefaultOpen))
    {
        auto& debugFlags = m_Context->GetSettings().DebugFlags;
        ImGui::Checkbox("Physics (Colliders)", &debugFlags.DrawColliders);
        if (debugFlags.DrawColliders)
        {
            const char* wireModes[] = {"Wireframe", "Solid", "Solid + Wireframe"};
            ImGui::Combo("Visual Mode", &debugFlags.SetCollisionWireframeMode, wireModes, 3);
        }
        ImGui::Checkbox("AABB Boxes", &debugFlags.DrawCollisionModelBox);
        ImGui::Checkbox("Lights", &debugFlags.DrawLights);
        ImGui::Checkbox("Spawn Zones", &debugFlags.DrawSpawnZones);
        // ImGui::Checkbox("Draw Grid", &debugFlags.DrawGrid);
        // if (debugFlags.DrawGrid)
        // {
        //     auto& grid = m_Context->GetSettings().Grid;
        //     ImGui::Indent(12.0f);
        //     ImGui::DragInt("Slices", &grid.Slices, 1, 4, 200);
        //     ImGui::DragFloat("Spacing", &grid.Spacing, 0.1f, 0.1f, 50.0f);
        //     ImGui::Unindent(12.0f);
        // }
    }

    ImGui::End();
}

} // namespace Chained
