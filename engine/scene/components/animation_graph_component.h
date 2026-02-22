#ifndef CH_ANIMATION_GRAPH_COMPONENT_H
#define CH_ANIMATION_GRAPH_COMPONENT_H

#include <string>
#include <map>
#include <memory>

namespace CHEngine
{
    class AnimationGraphAsset;

    struct AnimationGraphComponent
    {
        std::shared_ptr<AnimationGraphAsset> Graph;
        std::string CurrentState;
        
        // Parameters used to drive transitions (e.g., "Speed", "IsGrounded")
        std::map<std::string, float> Parameters;

        AnimationGraphComponent() = default;
        AnimationGraphComponent(const std::shared_ptr<AnimationGraphAsset>& graph)
            : Graph(graph) {}

        void SetParameter(const std::string& name, float value)
        {
            Parameters[name] = value;
        }

        float GetParameter(const std::string& name) const
        {
            if (auto it = Parameters.find(name); it != Parameters.end())
                return it->second;
            return 0.0f;
        }
    };
}

#endif // CH_ANIMATION_GRAPH_COMPONENT_H
