#include "Animation.h"

Animation::Animation() : m_animCount(0), m_animIndex(0), m_animCurrentFrame(0) {}

Animation::~Animation() = default;

void Animation::Update(Model &model)
{
    if (model.meshCount == 0 || model.materialCount == 0)
    {
        TraceLog(LOG_WARNING, "Model is null, cannot update animation");
        return;
    }
    if (m_animCount > 0 && m_animIndex < m_animCount)
    {
        ModelAnimation currentAnim = m_modelAnimations[m_animIndex];
        UpdateModelAnimation(model, currentAnim, m_animCurrentFrame);
        m_animCurrentFrame++;

        if (m_animCurrentFrame >= currentAnim.frameCount)
        {
            m_animCurrentFrame = 0; // Loop animation
        }
    }
}
void Animation::SetAnimationIndex(unsigned int index)
{
    if (index < m_animCount)
    {
        m_animIndex = index;
        m_animCurrentFrame = 0; // Reset frame when changing animation
    }
    else
    {
        TraceLog(LOG_WARNING, "Animation index %u out of bounds (max %d)", index, m_animCount - 1);
    }
}
void Animation::LoadAnimations(const std::string &path)
{
    if (path.empty())
    {
        TraceLog(LOG_WARNING, "Animation path is empty");
        return;
    }
    m_modelAnimations = LoadModelAnimations(path.c_str(), &m_animCount);
    if (m_modelAnimations == nullptr || m_animCount <= 0)
    {
        TraceLog(LOG_WARNING, "Failed to load animations from %s", path.c_str());
        m_animCount = 0;
        return;
    }
}