// ui_render_container.cpp
// Renders: TabBar, TabItem, CollapsingHeader, TreeNode, VerticalLayoutGroup
#include "ui_render_helpers.h"

namespace Chained
{

bool RenderTreeNode(TreeNodeData& node, UIControlComponent& wc, const ImVec2& pos, const ImVec2& size, ImFont* font, const TextStyle& textStyle)
{
    ImGui::SetCursorScreenPos(pos);
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth;
    if (node.DefaultOpen) flags |= ImGuiTreeNodeFlags_DefaultOpen;
    if (node.IsLeaf)      flags |= ImGuiTreeNodeFlags_Leaf;
    node.IsOpen = ImGui::TreeNodeEx(node.Label.c_str(), flags);
    if (node.IsOpen)
        ImGui::TreePop();
    return false;
}

bool RenderTabBar(TabBarData& bar, UIControlComponent& wc, const ImVec2& pos, const ImVec2& size)
{
    ImGui::SetCursorScreenPos(pos);
    ImGuiTabBarFlags flags = 0;
    if (bar.Reorderable)       flags |= ImGuiTabBarFlags_Reorderable;
    if (bar.AutoSelectNewTabs) flags |= ImGuiTabBarFlags_AutoSelectNewTabs;
    ImGui::BeginTabBar(bar.Label.c_str(), flags);
    ImGui::EndTabBar();
    return false;
}

bool RenderTabItem(TabItemData& item, UIControlComponent& wc, const ImVec2& pos, const ImVec2& size)
{
    ImGui::SetCursorScreenPos(pos);
    bool open = item.IsOpen;
    item.Selected = ImGui::BeginTabItem(item.Label.c_str(), item.IsOpen ? &open : nullptr);
    if (item.Selected)
        ImGui::EndTabItem();
    item.IsOpen = open;
    return false;
}

bool RenderCollapsingHeader(CollapsingHeaderData& header, UIControlComponent& wc, const ImVec2& pos, const ImVec2& size)
{
    ImGui::SetCursorScreenPos(pos);
    ImGuiTreeNodeFlags flags = 0;
    if (header.DefaultOpen) flags |= ImGuiTreeNodeFlags_DefaultOpen;
    header.IsOpen = ImGui::CollapsingHeader(header.Label.c_str(), flags);
    return false;
}

bool RenderVerticalLayoutGroup(const VerticalLayoutGroupData& layout, UIControlComponent& wc, const ImVec2& pos, const ImVec2& size)
{
    return false;
}

} // namespace Chained
