#include "animation_graph_panel.h"
#include "IconsFontAwesome6.h"
#include "animation_graph_serializer.h"
#include "editor_context.h"
#include "engine/scene/project.h"
#include <algorithm>

namespace CHEngine
{

AnimationGraphPanel::AnimationGraphPanel()
{
    m_Name = "Animation Graph";

    m_Options.mDisplayLinksAsCurves = true;
    m_Options.mNodeBorderColor = IM_COL32(80, 80, 80, 255);
    m_Options.mBackgroundColor = IM_COL32(30, 30, 30, 255);
    m_Options.mGridColor = IM_COL32(100, 100, 100, 40);
    m_Options.mGridColor2 = IM_COL32(100, 100, 100, 80);

    m_SceneTemplate.mHeaderColor = IM_COL32(160, 50, 50, 255);
    m_SceneTemplate.mBackgroundColor = IM_COL32(40, 40, 40, 255);
    m_SceneTemplate.mBackgroundColorOver = IM_COL32(50, 50, 50, 255);

    m_InputNames = {"Enter"};
    m_SceneTemplate.mInputCount = (uint8_t)m_InputNames.size();
    m_SceneTemplate.mInputNames = m_InputNames.data();

    m_OutputNames = {"Transition"};
    m_SceneTemplate.mOutputCount = (uint8_t)m_OutputNames.size();
    m_SceneTemplate.mOutputNames = m_OutputNames.data();

    Load();
}

void AnimationGraphPanel::OnImGuiRender(bool readOnly)
{
    if (!m_IsOpen)
    {
        return;
    }

    ImGui::SetNextWindowSize(ImVec2(1000, 700), ImGuiCond_FirstUseEver);
    if (ImGui::Begin(m_Name.c_str(), &m_IsOpen, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
    {
        // Toolbar
        if (ImGui::Button(ICON_FA_FLOPPY_DISK " Save"))
        {
            Save();
        }
        ImGui::SameLine();
        if (ImGui::Button(ICON_FA_FOLDER_OPEN " Load"))
        {
            Load();
        }
        ImGui::SameLine();
        if (ImGui::Button(ICON_FA_PLUS " Add State"))
        {
            AnimationState state;
            state.Name = "New State";
            state.Rect = ImRect(100, 100, 300, 180);
            m_Data.Nodes.push_back(state);
        }

        ImGui::Separator();

        // Layout: Graph (left) | Inspector (right)
        static float inspectorWidth = 300.0f;
        ImVec2 size = ImGui::GetContentRegionAvail();

        if (ImGui::BeginTable("##layout", 2, ImGuiTableFlags_Resizable))
        {
            ImGui::TableNextColumn();
            // Graph Area
            GraphEditor::Show(*this, m_Options, m_ViewState, true);

            ImGui::TableNextColumn();
            // Inspector Area
            DrawInspector();

            ImGui::EndTable();
        }
    }
    ImGui::End();
}

void AnimationGraphPanel::DrawInspector()
{
    ImGui::TextDisabled("State Inspector");
    ImGui::Separator();

    if (m_SelectedNodeIndex == -1 || m_SelectedNodeIndex >= (int)m_Data.Nodes.size())
    {
        ImGui::TextWrapped("Select a node to edit its properties.");
        return;
    }

    auto& node = m_Data.Nodes[m_SelectedNodeIndex];

    char nameBuf[128];
    strncpy(nameBuf, node.Name.c_str(), sizeof(nameBuf));
    if (ImGui::InputText("Name", nameBuf, sizeof(nameBuf)))
    {
        node.Name = nameBuf;
    }

    ImGui::Spacing();
    ImGui::Text(ICON_FA_FILM " Animation Clip");

    char pathBuf[256];
    strncpy(pathBuf, node.AnimationPath.c_str(), sizeof(pathBuf));
    if (ImGui::InputText("##Path", pathBuf, sizeof(pathBuf)))
    {
        node.AnimationPath = pathBuf;
    }

    if (ImGui::Button("Browse Projects Assets..."))
    {
        ImGui::OpenPopup("inspector_clip_popup");
    }

    if (ImGui::BeginPopup("inspector_clip_popup"))
    {
        if (m_AvailableAnimations.empty())
        {
            RefreshAvailableAnimations();
        }
        for (const auto& animPath : m_AvailableAnimations)
        {
            if (ImGui::Selectable(animPath.c_str()))
            {
                node.AnimationPath = animPath;
            }
        }
        if (ImGui::Selectable("Refresh List..."))
        {
            RefreshAvailableAnimations();
        }
        ImGui::EndPopup();
    }

    ImGui::Checkbox(ICON_FA_REPEAT " Looping", &node.IsLooping);

    ImGui::Separator();
    ImGui::Text(ICON_FA_RIGHT_FROM_BRACKET " Transitions");

    for (size_t i = 0; i < node.Transitions.size(); i++)
    {
        ImGui::PushID((int)i);
        char slotBuf[64];
        strncpy(slotBuf, node.Transitions[i].c_str(), sizeof(slotBuf));
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 30);
        if (ImGui::InputText("##slot", slotBuf, sizeof(slotBuf)))
        {
            node.Transitions[i] = slotBuf;
        }

        ImGui::SameLine();
        if (ImGui::Button(ICON_FA_TRASH_CAN))
        {
            node.Transitions.erase(node.Transitions.begin() + i);
            ImGui::PopID();
            break;
        }
        ImGui::PopID();
    }

    if (ImGui::Button(ICON_FA_PLUS " Add Condition"))
    {
        node.Transitions.push_back("Condition");
    }

    ImGui::Spacing();
    ImGui::Separator();
    if (ImGui::Button(ICON_FA_TRASH " Delete State", ImVec2(-1, 0)))
    {
        m_Data.Nodes.erase(m_Data.Nodes.begin() + m_SelectedNodeIndex);
        m_Data.Links.erase(std::remove_if(m_Data.Links.begin(), m_Data.Links.end(),
                                          [&](const AnimationTransition& link) {
                                              return link.FromState == (size_t)m_SelectedNodeIndex ||
                                                     link.ToState == (size_t)m_SelectedNodeIndex;
                                          }),
                           m_Data.Links.end());

        for (auto& link : m_Data.Links)
        {
            if (link.FromState > (size_t)m_SelectedNodeIndex)
            {
                link.FromState--;
            }
            if (link.ToState > (size_t)m_SelectedNodeIndex)
            {
                link.ToState--;
            }
        }
        m_SelectedNodeIndex = -1;
    }
}

void AnimationGraphPanel::SelectNode(GraphEditor::NodeIndex nodeIndex, bool selected)
{
    if (nodeIndex < m_Data.Nodes.size())
    {
        m_Data.Nodes[nodeIndex].Selected = selected;
        if (selected)
        {
            m_SelectedNodeIndex = (int)nodeIndex;
        }
        else if (m_SelectedNodeIndex == (int)nodeIndex)
        {
            m_SelectedNodeIndex = -1;
        }
    }
}

void AnimationGraphPanel::MoveSelectedNodes(const ImVec2 delta)
{
    for (auto& node : m_Data.Nodes)
    {
        if (node.Selected)
        {
            node.Rect.Min += delta;
            node.Rect.Max += delta;
        }
    }
}

void AnimationGraphPanel::AddLink(GraphEditor::NodeIndex inputNodeIndex, GraphEditor::SlotIndex inputSlotIndex,
                                  GraphEditor::NodeIndex outputNodeIndex, GraphEditor::SlotIndex outputSlotIndex)
{
    m_Data.Links.push_back({outputNodeIndex, outputSlotIndex, inputNodeIndex, inputSlotIndex});
}

void AnimationGraphPanel::DelLink(GraphEditor::LinkIndex linkIndex)
{
    if (linkIndex < m_Data.Links.size())
    {
        m_Data.Links.erase(m_Data.Links.begin() + linkIndex);
    }
}

void AnimationGraphPanel::RefreshAvailableAnimations()
{
    if (!Project::GetActive())
    {
        return;
    }

    m_AvailableAnimations.clear();
    std::filesystem::path animDir = Project::GetActive()->GetProjectDirectory() / "assets";
    if (std::filesystem::exists(animDir))
    {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(animDir))
        {
            if (entry.is_regular_file())
            {
                auto ext = entry.path().extension().string();
                if (ext == ".fbx" || ext == ".gltf" || ext == ".anim" || ext == ".glb")
                {
                    auto relPath =
                        std::filesystem::relative(entry.path(), Project::GetActive()->GetProjectDirectory() / "assets");
                    m_AvailableAnimations.push_back(relPath.generic_string());
                }
            }
        }
    }
}

void AnimationGraphPanel::CustomDraw(ImDrawList* drawList, ImRect rectangle, GraphEditor::NodeIndex nodeIndex)
{
    if (nodeIndex >= m_Data.Nodes.size())
    {
        return;
    }
    auto& node = m_Data.Nodes[nodeIndex];

    ImGui::SetCursorScreenPos(rectangle.Min + ImVec2(10, 10));
    ImGui::BeginGroup();

    ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "%s", node.Name.c_str());

    std::string clipName =
        node.AnimationPath.empty() ? "None" : std::filesystem::path(node.AnimationPath).filename().string();
    ImGui::TextDisabled(ICON_FA_FILM " %s", clipName.c_str());

    if (node.IsLooping)
    {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.4f, 0.8f, 0.4f, 1.0f), ICON_FA_REPEAT);
    }

    ImGui::EndGroup();
}

void AnimationGraphPanel::RightClick(GraphEditor::NodeIndex nodeIndex, GraphEditor::SlotIndex slotIndexInput,
                                     GraphEditor::SlotIndex slotIndexOutput)
{
    // Right click handled by selection and inspector now
}

const GraphEditor::Template AnimationGraphPanel::GetTemplate(GraphEditor::TemplateIndex index)
{
    if (index >= m_Data.Nodes.size())
    {
        return m_SceneTemplate;
    }

    const auto& node = m_Data.Nodes[index];
    GraphEditor::Template nodeTemplate = m_SceneTemplate;
    nodeTemplate.mOutputCount = (uint8_t)node.Transitions.size();

    static std::vector<const char*> outNames;
    outNames.clear();
    for (const auto& s : node.Transitions)
    {
        outNames.push_back(s.c_str());
    }
    nodeTemplate.mOutputNames = outNames.data();

    return nodeTemplate;
}

const size_t AnimationGraphPanel::GetNodeCount()
{
    return m_Data.Nodes.size();
}

const GraphEditor::Node AnimationGraphPanel::GetNode(GraphEditor::NodeIndex index)
{
    const auto& node = m_Data.Nodes[index];
    return {node.Name.c_str(), (GraphEditor::TemplateIndex)index, node.Rect, node.Selected};
}

const size_t AnimationGraphPanel::GetLinkCount()
{
    return m_Data.Links.size();
}

const GraphEditor::Link AnimationGraphPanel::GetLink(GraphEditor::LinkIndex index)
{
    const auto& link = m_Data.Links[index];
    return {link.FromState, link.FromSlot, link.ToState, link.ToSlot};
}

void AnimationGraphPanel::Save()
{
    if (!Project::GetActive())
    {
        return;
    }
    std::filesystem::path path =
        Project::GetActive()->GetProjectDirectory() / "assets" / "animations" / "default.chanim";
    AnimationGraphSerializer::Save(path, m_Data);
}

void AnimationGraphPanel::Load()
{
    if (!Project::GetActive())
    {
        return;
    }
    std::filesystem::path path =
        Project::GetActive()->GetProjectDirectory() / "assets" / "animations" / "default.chanim";
    AnimationGraphSerializer::Load(path, m_Data);
}

} // namespace CHEngine
