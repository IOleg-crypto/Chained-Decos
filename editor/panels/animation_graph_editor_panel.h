#ifndef CH_ANIMATION_GRAPH_EDITOR_PANEL_H
#define CH_ANIMATION_GRAPH_EDITOR_PANEL_H

#include "editor/panels/panel.h"
#include "engine/graphics/animation_graph_asset.h"
#include "GraphEditor.h"
#include <memory>

namespace CHEngine
{
    class AnimationGraphEditorPanel : public Panel
    {
    public:
        AnimationGraphEditorPanel();
        virtual ~AnimationGraphEditorPanel() = default;

        virtual void OnImGuiRender(bool readOnly = false) override;
        
        void SetGraph(const std::shared_ptr<AnimationGraphAsset>& graph);

    private:
        struct GraphDelegate : public GraphEditor::Delegate
        {
            AnimationGraphEditorPanel* Parent;
            std::shared_ptr<AnimationGraphAsset> Graph;

            GraphDelegate(AnimationGraphEditorPanel* parent) : Parent(parent) {}

            virtual bool AllowedLink(GraphEditor::NodeIndex from, GraphEditor::NodeIndex to) override;
            virtual void SelectNode(GraphEditor::NodeIndex nodeIndex, bool selected) override;
            virtual void MoveSelectedNodes(const ImVec2 delta) override;
            virtual void AddLink(GraphEditor::NodeIndex inputNodeIndex, GraphEditor::SlotIndex inputSlotIndex, GraphEditor::NodeIndex outputNodeIndex, GraphEditor::SlotIndex outputSlotIndex) override;
            virtual void DelLink(GraphEditor::LinkIndex linkIndex) override;
            virtual void CustomDraw(ImDrawList* drawList, ImRect rectangle, GraphEditor::NodeIndex nodeIndex) override;
            virtual void RightClick(GraphEditor::NodeIndex nodeIndex, GraphEditor::SlotIndex slotIndexInput, GraphEditor::SlotIndex slotIndexOutput) override;

            virtual const size_t GetTemplateCount() override;
            virtual const GraphEditor::Template GetTemplate(GraphEditor::TemplateIndex index) override;
            virtual const size_t GetNodeCount() override;
            virtual const GraphEditor::Node GetNode(GraphEditor::NodeIndex index) override;
            virtual const size_t GetLinkCount() override;
            virtual const GraphEditor::Link GetLink(GraphEditor::LinkIndex index) override;
        };

        std::shared_ptr<AnimationGraphAsset> m_ActiveGraph;
        GraphDelegate m_Delegate;
        GraphEditor::Options m_Options;
        GraphEditor::ViewState m_ViewState;
        
        // Internal state for GraphEditor mapping
        struct EditorNode {
            std::string StateName;
            ImVec2 Position;
        };
        std::vector<EditorNode> m_EditorNodes;
        
        void RebuildEditorState();
    };
}

#endif // CH_ANIMATION_GRAPH_EDITOR_PANEL_H
