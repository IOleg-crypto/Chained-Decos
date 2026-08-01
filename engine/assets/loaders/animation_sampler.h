#ifndef CH_ANIMATION_SAMPLER_H
#define CH_ANIMATION_SAMPLER_H

#include "engine/assets/model_data.h"
#include <assimp/scene.h>
#include <string>
#include <unordered_map>
#include <vector>

namespace Chained
{

class AnimationSampler
{
public:
    static void Process(const aiScene* scene, int samplingFPS, const std::vector<std::string>& nodeNames,
                        const std::unordered_map<std::string, int>& nameToIndex,
                        std::vector<AnimationData>& animations);

private:
    static void SampleAnimation(aiAnimation* anim, int samplingFPS, const aiScene* scene,
                                const std::vector<std::string>& nodeNames,
                                const std::unordered_map<std::string, int>& nameToIndex, AnimationData& out);
};

} // namespace Chained

#endif // CH_ANIMATION_SAMPLER_H
