#ifndef CH_PHYSICS_COMPONENTS_H
#define CH_PHYSICS_COMPONENTS_H

#include "engine/assets/asset.h"
#include "engine/graphics/api/model_data.h"
#include "engine/physics/iphysics_world.h"
#include "engine/reflection/reflection_rfl.h"
#include <glm/glm.hpp>
#include <string>


namespace Chained
{

struct ColliderComponent
{
    // Serialized fields
    ColliderType Type = ColliderType::Box;

    glm::vec3 Size = {0.5f, 0.5f, 0.5f};
    glm::vec3 Offset = {0.0f, 0.0f, 0.0f};
    float Radius = 0.5f;
    float Height = 1.0f;

    float Friction = 0.5f;
    float Restitution = 0.0f;

    bool IsTrigger = false;
    bool Enabled = true;
    bool AutoCalculate = false;

    std::string ModelPath;
    AssetHandle ModelHandle = 0;

    // Runtime state (not serialized - excluded by ReflectBridge)
    bool IsColliding = false;

    static const char* GetStaticName()
    {
        return "ColliderComponent";
    }
    static BoundingBox CalculateWorldAABB(const ColliderComponent& collider, const glm::mat4& worldTransform);

    
    struct UI
    {
        UIMeta Type = {.Tooltip = "Форма геометрії коллайдера", .Hint = PropertyMeta::WidgetHint::Enum};
        UIMeta Size = {.Speed = 0.05f, .Tooltip = "Розміри Box-коллайдера"};
        UIMeta Offset = {.Speed = 0.05f, .Tooltip = "Зміщення центра коллайдера відносно локального нуля сутності"};
        UIMeta Radius = {.Min = 0.0f, .Max = 500.0f, .Speed = 0.05f, .Tooltip = "Радіус для Sphere або Capsule"};
        UIMeta Height = {.Min = 0.0f, .Max = 500.0f, .Speed = 0.05f, .Tooltip = "Повна висота капсули"};
        UIMeta Friction = {.Min = 0.0f, .Max = 1.0f, .Speed = 0.01f, .Tooltip = "Коефіцієнт тертя матеріалу"};
        UIMeta Restitution = {
            .Min = 0.0f, .Max = 1.0f, .Speed = 0.01f, .Tooltip = "Пружність матеріалу (відскок при ударі)"};
        UIMeta IsTrigger = {.Tooltip =
                                "Якщо увімкнено, коллайдер лише реєструє перетини, але не заважає руху фізичних тіл"};
        UIMeta AutoCalculate = {.Tooltip = "Автоматично розрахувати розміри коллайдера на основі ModelComponent"};
        UIMeta ModelPath = {.Hint = PropertyMeta::WidgetHint::FilePicker,
                            .Extensions = ".glb,.gltf,.obj",
                            .Tooltip = "Шлях до 3D-моделі для Mesh-коллайдера"};
    };
};

CH_MARK_RFL(ColliderComponent);

struct RigidBodyComponent
{
    // Serialized fields
    enum class BodyType
    {
        Static,
        Dynamic,
        Kinematic
    };
    BodyType Type = BodyType::Dynamic;
    float Mass = 1.0f;
    float LinearDamping = 0.01f;
    float AngularDamping = 0.05f;
    bool UseGravity = true;
    bool IsFixedRotation = false;
    bool IsKinematic = false;

    // Runtime state (not serialized - excluded by ReflectBridge)
    PhysicsBodyHandle Handle = kInvalidPhysicsBody;
    glm::vec3 Velocity = glm::vec3(0.0f);
    bool IsGrounded = false;

    static const char* GetStaticName()
    {
        return "RigidBodyComponent";
    }

    
    struct UI
    {
        UIMeta Type = {
            .Tooltip = "Режим поведінки тіла: Static (нерухоме), Dynamic (повна фізика), Kinematic (керується кодом)"};
        UIMeta Mass = {.Min = 0.0f, .Max = 100000.0f, .Speed = 0.1f, .Tooltip = "Маса фізичного об'єкта"};
        UIMeta LinearDamping = {
            .Min = 0.0f, .Max = 1.0f, .Speed = 0.005f, .Tooltip = "Опір повітря для лінійного руху"};
        UIMeta AngularDamping = {
            .Min = 0.0f, .Max = 1.0f, .Speed = 0.005f, .Tooltip = "Опір повітря для обертання об'єкта"};
        UIMeta UseGravity = {.Tooltip = "Чи діє на це тіло глобальна гравітація сцени"};
        UIMeta IsFixedRotation = {.Tooltip = "Блокує будь-яке обертання об'єкта під дією фізичних сил"};
        UIMeta IsKinematic = {.Tooltip = "Дублюючий прапорець статусу кінематичності для внутрішніх розрахунків"};
    };
};

CH_MARK_RFL(RigidBodyComponent);

} // namespace Chained

#endif // CH_PHYSICS_COMPONENTS_H