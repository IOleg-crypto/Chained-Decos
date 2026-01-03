#ifndef CH_COMPONENTS_H
#define CH_COMPONENTS_H

#include <raylib.h>
#include <raymath.h>
#include <string>


namespace CH
{
struct TagComponent
{
    std::string Tag;

    TagComponent() = default;
    TagComponent(const TagComponent &) = default;
    TagComponent(const std::string &tag) : Tag(tag)
    {
    }
};

struct TransformComponent
{
    Vector3 Translation = {0.0f, 0.0f, 0.0f};
    Vector3 Rotation = {0.0f, 0.0f, 0.0f};
    Vector3 Scale = {1.0f, 1.0f, 1.0f};

    TransformComponent() = default;
    TransformComponent(const TransformComponent &) = default;
    TransformComponent(const Vector3 &translation) : Translation(translation)
    {
    }

    Matrix GetTransform() const
    {
        Matrix rotation = MatrixRotateXYZ(Rotation);

        return MatrixMultiply(MatrixMultiply(MatrixScale(Scale.x, Scale.y, Scale.z), rotation),
                              MatrixTranslate(Translation.x, Translation.y, Translation.z));
    }
};
} // namespace CH

#endif // CH_COMPONENTS_H
