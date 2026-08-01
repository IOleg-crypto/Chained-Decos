#ifndef CH_ASSIMP_HELPERS_H
#define CH_ASSIMP_HELPERS_H

#include <assimp/types.h>
#include <assimp/anim.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>

namespace Chained
{

inline glm::mat4 ToMat4(const aiMatrix4x4& m)
{
    return glm::transpose(glm::make_mat4(&m.a1));
}

inline glm::vec3 ToVec3(const aiVector3D& v)
{
    return {v.x, v.y, v.z};
}

inline glm::quat ToQuat(const aiQuaternion& q)
{
    return {q.w, q.x, q.y, q.z};
}

inline glm::vec4 ToColor(const aiColor4D& c)
{
    return {c.r, c.g, c.b, c.a};
}

template <typename KeyT, typename T, typename ConvertFn, typename InterpolateFn>
inline T InterpolateKeys(double time, const KeyT* keys, unsigned int count, unsigned int& lastKey, const T& defaultVal,
                         ConvertFn convert, InterpolateFn lerpFn)
{
    if (count == 0)
    {
        return defaultVal;
    }
    if (count == 1)
    {
        return convert(keys[0].mValue);
    }

    unsigned int p1 = lastKey, p2 = lastKey;
    for (unsigned int k = lastKey; k < count - 1; ++k)
    {
        if (time < keys[k + 1].mTime)
        {
            p1 = k;
            p2 = k + 1;
            lastKey = k;
            break;
        }
        p1 = k;
        p2 = k + 1;
    }

    if (time >= keys[count - 1].mTime)
    {
        lastKey = count - 1;
        return convert(keys[count - 1].mValue);
    }

    double dt = keys[p2].mTime - keys[p1].mTime;
    float factor = (dt > 0.0) ? (float)((time - keys[p1].mTime) / dt) : 0.0f;
    return lerpFn(convert(keys[p1].mValue), convert(keys[p2].mValue), factor);
}

inline glm::vec3 InterpolatePosition(double time, const aiNodeAnim* channel, unsigned int& lastKey,
                                     const glm::vec3& defaultVal)
{
    return InterpolateKeys(time, channel->mPositionKeys, channel->mNumPositionKeys, lastKey, defaultVal, ToVec3,
                           [](const glm::vec3& a, const glm::vec3& b, float f) { return glm::mix(a, b, f); });
}

inline glm::quat InterpolateRotation(double time, const aiNodeAnim* channel, unsigned int& lastKey,
                                     const glm::quat& defaultVal)
{
    return InterpolateKeys(time, channel->mRotationKeys, channel->mNumRotationKeys, lastKey, defaultVal, ToQuat,
                           [](const glm::quat& a, const glm::quat& b, float f) {
                               aiQuaternion result;
                               aiQuaternion::Interpolate(result, aiQuaternion(a.w, a.x, a.y, a.z),
                                                         aiQuaternion(b.w, b.x, b.y, b.z), f);
                               return ToQuat(result);
                           });
}

inline glm::vec3 InterpolateScale(double time, const aiNodeAnim* channel, unsigned int& lastKey,
                                  const glm::vec3& defaultVal)
{
    return InterpolateKeys(time, channel->mScalingKeys, channel->mNumScalingKeys, lastKey, defaultVal, ToVec3,
                           [](const glm::vec3& a, const glm::vec3& b, float f) { return glm::mix(a, b, f); });
}

} // namespace Chained

#endif // CH_ASSIMP_HELPERS_H
