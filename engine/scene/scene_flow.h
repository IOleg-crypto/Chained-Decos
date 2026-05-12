#ifndef CH_SCENE_FLOW_H
#define CH_SCENE_FLOW_H

#include <string>
#include <vector>
#include <map>

namespace CHEngine {

struct SceneFlowLink {
    std::string TriggerName;
    std::string TargetScenePath;
};

struct SceneFlowNode {
    std::string ScenePath;
    std::vector<SceneFlowLink> OutgoingLinks;
};

class SceneFlowGraph {
public:
    void AddTransition(const std::string& fromScene, const std::string& trigger, const std::string& toScene)
    {
        m_Nodes[fromScene].OutgoingLinks.push_back({ trigger, toScene });
    }

    std::string GetTransition(const std::string& currentScene, const std::string& trigger) const
    {
        auto it = m_Nodes.find(currentScene);
        if (it != m_Nodes.end())
        {
            for (const auto& link : it->second.OutgoingLinks)
            {
                if (link.TriggerName == trigger)
                    return link.TargetScenePath;
            }
        }
        return "";
    }

    const std::map<std::string, SceneFlowNode>& GetNodes() const { return m_Nodes; }

private:
    std::map<std::string, SceneFlowNode> m_Nodes;
};

} // namespace CHEngine

#endif // CH_SCENE_FLOW_H
