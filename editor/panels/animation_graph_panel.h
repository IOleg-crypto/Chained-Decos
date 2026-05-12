#pragma once

#include "panel.h"
#include "include/imguizmo/GraphEditor.h"
#include <vector>
#include <string>
#include <filesystem>

#include "engine/scene/animation_graph_data.h"
#include "panels/animation_graph_serializer.h"

namespace CHEngine {

class AnimationGraphPanel : public Panel, public GraphEditor::Delegate {
public:
    AnimationGraphPanel();
    virtual ~AnimationGraphPanel() = default;

    virtual void OnImGuiRender(bool readOnly = false) override;

    // GraphEditor::Delegate implementation
    virtual bool AllowedLink(GraphEditor::NodeIndex from, GraphEditor::NodeIndex to) override { return true; }
    virtual void SelectNode(GraphEditor::NodeIndex nodeIndex, bool selected) override;
    virtual void MoveSelectedNodes(const ImVec2 delta) override;
    virtual void AddLink(GraphEditor::NodeIndex inputNodeIndex, GraphEditor::SlotIndex inputSlotIndex, GraphEditor::NodeIndex outputNodeIndex, GraphEditor::SlotIndex outputSlotIndex) override;
    virtual void DelLink(GraphEditor::LinkIndex linkIndex) override;
    virtual void CustomDraw(ImDrawList* drawList, ImRect rectangle, GraphEditor::NodeIndex nodeIndex) override;
    virtual void RightClick(GraphEditor::NodeIndex nodeIndex, GraphEditor::SlotIndex slotIndexInput, GraphEditor::SlotIndex slotIndexOutput) override;

    virtual const size_t GetTemplateCount() override { return 1; }
    virtual const GraphEditor::Template GetTemplate(GraphEditor::TemplateIndex index) override;
    virtual const size_t GetNodeCount() override;
    virtual const GraphEditor::Node GetNode(GraphEditor::NodeIndex index) override;
    virtual const size_t GetLinkCount() override;
    virtual const GraphEditor::Link GetLink(GraphEditor::LinkIndex index) override;

private:
    void Save();
    void Load();

    void RefreshAvailableAnimations();
    void DrawInspector();

    AnimationGraphData m_Data;
    GraphEditor::Options m_Options;
    GraphEditor::ViewState m_ViewState;
    
    int m_SelectedNodeIndex = -1;
    std::vector<std::string> m_AvailableAnimations;

    GraphEditor::Template m_SceneTemplate;
    std::vector<const char*> m_InputNames;
    std::vector<const char*> m_OutputNames;
};

} // namespace CHEngine
