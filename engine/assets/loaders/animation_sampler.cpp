#include "engine/assets/loaders/animation_sampler.h"
#include "engine/assets/loaders/assimp_helpers.h"
#include "engine/common/thread_pool.h"
#include "engine/core/log.h"
#include "engine/core/profiler.h"
#include "engine/core/service_locator.h"
#include <algorithm>
#include <cmath>
#include <future>

namespace Chained
{

void AnimationSampler::Process(const aiScene* scene, int samplingFPS, const std::vector<std::string>& nodeNames,
                               const std::unordered_map<std::string, int>& nameToIndex,
                               std::vector<AnimationData>& animations)
{
    CH_PROFILE_SCOPE("AnimationSampler::Process");
    animations.resize(scene->mNumAnimations);
    for (unsigned int a = 0; a < scene->mNumAnimations; ++a)
    {
        SampleAnimation(scene->mAnimations[a], samplingFPS, scene, nodeNames, nameToIndex, animations[a]);
    }
}

void AnimationSampler::SampleAnimation(aiAnimation* anim, int samplingFPS, const aiScene* scene,
                                       const std::vector<std::string>& nodeNames,
                                       const std::unordered_map<std::string, int>& nameToIndex, AnimationData& out)
{
    out.name = anim->mName.C_Str();
    if (out.name.empty())
    {
        return;
    }

    const double ticksPerSecond =
        (anim->mTicksPerSecond > 0.0) ? anim->mTicksPerSecond : (double)std::max(1, samplingFPS);
    out.frameRate = (float)std::max(1, samplingFPS);

    double durationTicks = anim->mDuration;
    if (durationTicks == 0.0)
    {
        for (unsigned int c = 0; c < anim->mNumChannels; ++c)
        {
            if (anim->mChannels[c]->mNumPositionKeys > 0)
            {
                durationTicks = std::max(
                    durationTicks, anim->mChannels[c]->mPositionKeys[anim->mChannels[c]->mNumPositionKeys - 1].mTime);
            }
            if (anim->mChannels[c]->mNumRotationKeys > 0)
            {
                durationTicks = std::max(
                    durationTicks, anim->mChannels[c]->mRotationKeys[anim->mChannels[c]->mNumRotationKeys - 1].mTime);
            }
            if (anim->mChannels[c]->mNumScalingKeys > 0)
            {
                durationTicks = std::max(
                    durationTicks, anim->mChannels[c]->mScalingKeys[anim->mChannels[c]->mNumScalingKeys - 1].mTime);
            }
        }
    }

    const double durationSeconds = (ticksPerSecond > 0.0) ? (durationTicks / ticksPerSecond) : 0.0;
    out.frameCount = std::max(1, (int)std::ceil(durationSeconds * (double)out.frameRate) + 1);
    out.boneCount = (int)nodeNames.size();
    out.framePoses.resize(out.frameCount * out.boneCount);
    const double ticksPerFrame = ticksPerSecond / (double)out.frameRate;

    std::vector<TransformData> bindPoses(out.boneCount);
    for (int b = 0; b < out.boneCount; ++b)
    {
        aiNode* node = scene->mRootNode->FindNode(nodeNames[b].c_str());
        if (node)
        {
            aiVector3D p, s;
            aiQuaternion r;
            node->mTransformation.Decompose(s, r, p);
            bindPoses[b] = {ToVec3(p), ToQuat(r), ToVec3(s)};
        }
        else
        {
            bindPoses[b] = {glm::vec3(0), glm::quat(1, 0, 0, 0), glm::vec3(1)};
        }
    }

    for (int f = 0; f < out.frameCount; ++f)
    {
        for (int b = 0; b < out.boneCount; ++b)
        {
            out.framePoses[f * out.boneCount + b] = bindPoses[b];
        }
    }

    std::vector<std::future<void>> animFutures;
    animFutures.reserve(anim->mNumChannels);
    for (unsigned int c = 0; c < anim->mNumChannels; ++c)
    {
        aiNodeAnim* channel = anim->mChannels[c];
        auto boneIt = nameToIndex.find(channel->mNodeName.C_Str());
        if (boneIt == nameToIndex.end())
        {
            continue;
        }

        const int boneIdx = boneIt->second;
        if (auto* tp = ServiceLocator::TryGet<ThreadPool>())
        {
            animFutures.push_back(tp->Enqueue([&out, channel, boneIdx, &bindPoses, ticksPerFrame]() {
                unsigned int lastPosKey = 0, lastRotKey = 0, lastSclKey = 0;
                for (int f = 0; f < out.frameCount; ++f)
                {
                    double time = (double)f * ticksPerFrame;
                    glm::vec3 pos = InterpolatePosition(time, channel, lastPosKey, bindPoses[boneIdx].translation);
                    glm::quat rot = InterpolateRotation(time, channel, lastRotKey, bindPoses[boneIdx].rotation);
                    glm::vec3 scale = InterpolateScale(time, channel, lastSclKey, bindPoses[boneIdx].scale);
                    out.framePoses[f * out.boneCount + boneIdx] = {pos, rot, scale};
                }
            }));
        }
    }
    for (auto& f : animFutures)
    {
        f.get();
    }
    CH_CORE_INFO("AnimationSampler: Loaded animation '{}' ({} frames, {} fps, {} channels)", out.name, out.frameCount,
                 out.frameRate, anim->mNumChannels);
}

} // namespace Chained
