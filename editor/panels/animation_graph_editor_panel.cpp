#include "animation_graph_editor_panel.h"
#include <imgui.h>

namespace CHEngine
{
    AnimationGraphEditorPanel::AnimationGraphEditorPanel()
        : m_Delegate(this)
    {
        m_Name = "Animation Graph Editor";
        
        m_Options.mBackgroundColor = IM_COL32(30, 30, 30, 255);
        m_Options.mGridColor = IM_COL32(50, 50, 50, 100);
        m_Options.mDisplayLinksAsCurves = true;
    }

    void AnimationGraphEditorPanel::OnImGuiRender(bool readOnly)
    {
        if (!m_IsOpen) return;

        ImGui::Begin(m_Name.c_str(), &m_IsOpen);

        if (!m_ActiveGraph)
        {
            ImGui::Text("No active graph selected.");
            ImGui::End();
            return;
        }

        // --- Toolbar ---
        if (ImGui::Button("Add State"))
        {
            // Simple state name generation
            std::string name = "NewState_" + std::to_string(m_ActiveGraph->GetStates().size());
            m_ActiveGraph->AddState({name});
            RebuildEditorState();
        }
        ImGui::SameLine();
        if (ImGui::Button("Fit View"))
        {
            // Logic to fit view could go here
        }

        ImGui::Separator();

        // --- Graph Editor ---
        GraphEditor::Show(m_Delegate, m_Options, m_ViewState, !readOnly);

        ImGui::End();
    }

    void AnimationGraphEditorPanel::SetGraph(const std::shared_ptr<AnimationGraphAsset>& graph)
    {
        m_ActiveGraph = graph;
        if (m_ActiveGraph)
        {
            RebuildEditorState();
        }
    }

    void AnimationGraphEditorPanel::RebuildEditorState()
    {
        m_EditorNodes.clear();
        if (!m_ActiveGraph) return;

        for (auto& [name, state] : m_ActiveGraph->GetStates())
        {
            m_EditorNodes.push_back({name, {state.EditorPosX, state.EditorPosY}});
        }
    }

    // --- Delegate Implementation ---

    bool AnimationGraphEditorPanel::GraphDelegate::AllowedLink(GraphEditor::NodeIndex from, GraphEditor::NodeIndex to)
    {
        return from != to;
    }

    void AnimationGraphEditorPanel::GraphDelegate::SelectNode(GraphEditor::NodeIndex nodeIndex, bool selected)
    {
        // Handle selection if needed
    }

    void AnimationGraphEditorPanel::GraphDelegate::MoveSelectedNodes(const ImVec2 delta)
    {
        for (size_t i = 0; i < Parent->m_EditorNodes.size(); i++)
        {
            // In a real implementation, you'd check which nodes are selected
            // For now, let's assume we update positions for nodes we find
            auto& node = Parent->m_EditorNodes[i];
            auto* state = const_cast<AnimationState*>(Graph->GetState(node.StateName));
            if (state)
            {
                state->EditorPosX += delta.x;
                state->EditorPosY += delta.y;
                node.Position.x = state->EditorPosX;
                node.Position.y = state->EditorPosY;
            }
        }
    }

    void AnimationGraphEditorPanel::GraphDelegate::AddLink(GraphEditor::NodeIndex inputNodeIndex, GraphEditor::SlotIndex inputSlotIndex, GraphEditor::NodeIndex outputNodeIndex, GraphEditor::SlotIndex outputSlotIndex)
    {
        if (inputNodeIndex >= Parent->m_EditorNodes.size() || outputNodeIndex >= Parent->m_EditorNodes.size())
            return;

        const auto& fromName = Parent->m_EditorNodes[inputNodeIndex].StateName; // Output slot of node i
        const auto& toName = Parent->m_EditorNodes[outputNodeIndex].StateName;  // Input slot of node o
        
        // In GraphEditor, slots are indexed. Let's say:
        // Slot 0 of node A is Output (Transition)
        // Slot 0 of node B is Input (Target)
        
        auto* state = const_cast<AnimationState*>(Graph->GetState(fromName));
        if (state)
        {
            AnimationTransition trans;
            trans.TargetState = toName;
            state->Transitions.push_back(trans);
        }
    }

    void AnimationGraphEditorPanel::GraphDelegate::DelLink(GraphEditor::LinkIndex linkIndex)
    {
        // Implement link deletion logic
    }

    void AnimationGraphEditorPanel::GraphDelegate::CustomDraw(ImDrawList* drawList, ImRect rectangle, GraphEditor::NodeIndex nodeIndex)
    {
        // Draw extra info like animation name inside the node
    }

    void AnimationGraphEditorPanel::GraphDelegate::RightClick(GraphEditor::NodeIndex nodeIndex, GraphEditor::SlotIndex slotIndexInput, GraphEditor::SlotIndex slotIndexOutput)
    {
        // Context menu for deleting states or adding conditions
    }

    const size_t AnimationGraphEditorPanel::GraphDelegate::GetTemplateCount()
    {
        return 1;
    }

    const GraphEditor::Template AnimationGraphEditorPanel::GraphDelegate::GetTemplate(GraphEditor::TemplateIndex index)
    {
        static const char* inputNames[] = {"In"};
        static const char* outputNames[] = {"Out"};
        return {
            IM_COL32(160, 160, 180, 255), // Header
            IM_COL32(100, 100, 100, 255), // Background
            IM_COL32(110, 110, 110, 255), // Background Over
            1, inputNames, nullptr,
            1, outputNames, nullptr
        };
    }

    const size_t AnimationGraphEditorPanel::GraphDelegate::GetNodeCount()
    {
        return Parent->m_EditorNodes.size();
    }

    const GraphEditor::Node AnimationGraphEditorPanel::GraphDelegate::GetNode(GraphEditor::NodeIndex index)
    {
        const auto& node = Parent->m_EditorNodes[index];
        return {
            node.StateName.c_str(),
            0, // Template index
            ImRect(node.Position, {node.Position.x + 150, node.Position.y + 100}), // Rect
            false // Selected
        };
    }

    const size_t AnimationGraphEditorPanel::GraphDelegate::GetLinkCount()
    {
        size_t count = 0;
        for (auto& [name, state] : Graph->GetStates())
        {
            count += state.Transitions.size();
        }
        return count;
    }

    const GraphEditor::Link AnimationGraphEditorPanel::GraphDelegate::GetLink(GraphEditor::LinkIndex index)
    {
        size_t current = 0;
        for (size_t i = 0; i < Parent->m_EditorNodes.size(); i++)
        {
            const auto& node = Parent->m_EditorNodes[i];
            const auto* state = Graph->GetState(node.StateName);
            if (current + state->Transitions.size() > index)
            {
                size_t transIdx = index - current;
                const auto& transition = state->Transitions[transIdx];
                
                // Find target node index
                size_t targetIdx = 0;
                for (size_t j = 0; j < Parent->m_EditorNodes.size(); j++)
                {
                    if (Parent->m_EditorNodes[j].StateName == transition.TargetState)
                    {
                        targetIdx = j;
                        break;
                    }
                }
                
                return {targetIdx, 0, i, 0}; // targetNode, inputSlot, sourceNode, outputSlot
            }
            current += state->Transitions.size();
        }
        return {0, 0, 0, 0};
    }
}
