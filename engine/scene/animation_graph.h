#ifndef CH_ANIMATION_GRAPH_H
#define CH_ANIMATION_GRAPH_H

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include "engine/assets/asset.h"

namespace Chained
{

enum class AnimConditionOp
{
    Equal,
    NotEqual,
    Greater,
    Less,
    GreaterOrEqual,
    LessOrEqual
};

struct AnimCondition
{
    std::string VariableName;
    AnimConditionOp Op = AnimConditionOp::Equal;
    float Value = 0.0f;

    bool Evaluate(const std::unordered_map<std::string, float>& variables) const
    {
        auto it = variables.find(VariableName);
        float currentVal = (it != variables.end()) ? it->second : 0.0f;

        switch (Op)
        {
        case AnimConditionOp::Equal:
            return currentVal == Value;
        case AnimConditionOp::NotEqual:
            return currentVal != Value;
        case AnimConditionOp::Greater:
            return currentVal > Value;
        case AnimConditionOp::Less:
            return currentVal < Value;
        case AnimConditionOp::GreaterOrEqual:
            return currentVal >= Value;
        case AnimConditionOp::LessOrEqual:
            return currentVal <= Value;
        }
        return false;
    }
};

struct AnimTransition
{
    int ID = 0;
    int SourceNodeID = 0;
    int TargetNodeID = 0;
    float BlendDuration = 0.2f;
    std::vector<AnimCondition> Conditions;
    bool HasExitTime = false; // If true, transition waits until animation finishes
    float ExitTime = 0.9f;    // Normalized time to transition at

    // ExitTime reference mode
    enum class ExitTimeMode
    {
        SourceAnimation, // ExitTime relative to source animation (current)
        TargetAnimation, // ExitTime relative to target animation
        AbsoluteTime     // ExitTime in seconds from state entry
    } ExitTimeMode = ExitTimeMode::SourceAnimation;

    int Priority = 0; // Higher priority = evaluated first
};

struct AnimNode
{
    int ID = 0;
    std::string Name = "New State";
    int AnimationIndex = 0;
    bool IsLooping = true;
    int StartFrame = 0;
    int EndFrame = -1; // -1 = use full animation length
    float Speed = 1.0f;

    // Editor-only position data
    float EditorX = 0.0f;
    float EditorY = 0.0f;
};

class AnimationGraphAsset : public Asset
{
public:
    AnimationGraphAsset()
        : Asset(AssetType::AnimationGraph)
    {
    }
    ~AnimationGraphAsset() override = default;

    static AssetType GetStaticType()
    {
        return AssetType::AnimationGraph;
    }

    std::vector<AnimNode> Nodes;
    std::vector<AnimTransition> Transitions;
    int EntryNodeID = -1;

    // Editor tracking
    int NextNodeID = 1;
    int NextLinkID = 1;

    AnimNode* FindNode(int id)
    {
        for (auto& node : Nodes)
        {
            if (node.ID == id)
            {
                return &node;
            }
        }
        return nullptr;
    }

    AnimTransition* FindTransition(int id)
    {
        for (auto& transition : Transitions)
        {
            if (transition.ID == id)
            {
                return &transition;
            }
        }
        return nullptr;
    }

    bool Save(const std::string& path);
};

} // namespace Chained
#endif
