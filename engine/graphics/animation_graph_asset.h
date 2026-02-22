#ifndef CH_ANIMATION_GRAPH_ASSET_H
#define CH_ANIMATION_GRAPH_ASSET_H

#include "engine/graphics/asset.h"
#include <map>
#include <string>
#include <vector>

namespace CHEngine
{
    enum class TransitionConditionOperator
    {
        GreaterThan,
        LessThan,
        Equals,
        NotEquals
    };

    struct TransitionCondition
    {
        std::string ParameterName;
        TransitionConditionOperator Operator;
        float Value;

        bool Evaluate(float currentValue) const
        {
            switch (Operator)
            {
                case TransitionConditionOperator::GreaterThan: return currentValue > Value;
                case TransitionConditionOperator::LessThan:    return currentValue < Value;
                case TransitionConditionOperator::Equals:      return currentValue == Value;
                case TransitionConditionOperator::NotEquals:   return currentValue != Value;
            }
            return false;
        }
    };

    struct AnimationTransition
    {
        std::string TargetState;
        std::vector<TransitionCondition> Conditions;
        float CrossfadeDuration = 0.2f;

        bool AreConditionsMet(const std::map<std::string, float>& parameters) const
        {
            if (Conditions.empty()) return true;
            for (const auto& cond : Conditions)
            {
                float val = 0.0f;
                if (auto it = parameters.find(cond.ParameterName); it != parameters.end())
                    val = it->second;
                
                if (!cond.Evaluate(val)) return false;
            }
            return true;
        }
    };

    struct AnimationState
    {
        std::string Name;
        int AnimationIndex = 0;
        float PlaybackSpeed = 1.0f;
        bool IsLooping = true;
        std::vector<AnimationTransition> Transitions;

        // Editor metadata
        float EditorPosX = 0.0f;
        float EditorPosY = 0.0f;
    };

    class AnimationGraphAsset : public Asset
    {
    public:
        AnimationGraphAsset() : Asset(AssetType::AnimationGraph) {}

        void AddState(const AnimationState& state)
        {
            m_States[state.Name] = state;
            if (m_DefaultState.empty()) m_DefaultState = state.Name;
        }

        const AnimationState* GetState(const std::string& name) const
        {
            auto it = m_States.find(name);
            return (it != m_States.end()) ? &it->second : nullptr;
        }

        const std::string& GetDefaultState() const { return m_DefaultState; }
        void SetDefaultState(const std::string& name) { m_DefaultState = name; }

        std::map<std::string, AnimationState>& GetStates() { return m_States; }
        const std::map<std::string, AnimationState>& GetStates() const { return m_States; }

    private:
        std::map<std::string, AnimationState> m_States;
        std::string m_DefaultState;
    };
}

#endif // CH_ANIMATION_GRAPH_ASSET_H
