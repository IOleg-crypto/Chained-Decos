#include "ViewportPicking.h"
#include "scene/resources/map/MapData.h"
#include <raymath.h>

namespace CHEngine
{

Ray ViewportPicking::GetMouseRay(ImVec2 mousePos, ImVec2 viewportPos, ImVec2 viewportSize,
                                 const Camera3D &camera)
{
    // Convert to viewport-local coordinates
    Vector2 localMouse = {mousePos.x - viewportPos.x, mousePos.y - viewportPos.y};

    // Clamp to viewport bounds
    localMouse.x = Clamp(localMouse.x, 0.0f, viewportSize.x);
    localMouse.y = Clamp(localMouse.y, 0.0f, viewportSize.y);

    // Convert to Normalized Device Coordinates (NDC) [-1, 1]
    // Hazel-style: proper NDC conversion with Y-flip for OpenGL
    float ndcX = (2.0f * localMouse.x) / viewportSize.x - 1.0f;
    float ndcY = 1.0f - (2.0f * localMouse.y) / viewportSize.y; // Flip Y for OpenGL

    // Calculate ray direction in view space
    float aspectRatio = viewportSize.x / viewportSize.y;
    float tanHalfFovy = tanf(camera.fovy * 0.5f * DEG2RAD);

    // View space ray direction
    Vector3 rayDir = {
        ndcX * aspectRatio * tanHalfFovy, ndcY * tanHalfFovy,
        -1.0f // Forward in view space
    };

    // Transform to world space
    Vector3 forward = Vector3Normalize(Vector3Subtract(camera.target, camera.position));
    Vector3 right = Vector3Normalize(Vector3CrossProduct(forward, camera.up));
    Vector3 up = Vector3CrossProduct(right, forward);

    Vector3 worldRayDir =
        Vector3Normalize({rayDir.x * right.x + rayDir.y * up.x + rayDir.z * forward.x,
                          rayDir.x * right.y + rayDir.y * up.y + rayDir.z * forward.y,
                          rayDir.x * right.z + rayDir.y * up.z + rayDir.z * forward.z});

    return {camera.position, worldRayDir};
}

RayCollision ViewportPicking::TestObjectCollision(const Ray &ray, const MapObjectData &obj)
{
    // Transform ray to object's local space
    Matrix transform = MatrixTranslate(obj.position.x, obj.position.y, obj.position.z);
    Matrix invTransform = MatrixInvert(transform);

    Vector3 localRayPos = Vector3Transform(ray.position, invTransform);
    Vector3 localRayDir = Vector3Transform(ray.direction, invTransform);
    Ray localRay = {localRayPos, Vector3Normalize(localRayDir)};

    // Calculate bounding box based on object type
    BoundingBox localBox;

    switch (obj.type)
    {
    case MapObjectType::CUBE:
        localBox = {Vector3Scale(obj.scale, -0.5f), Vector3Scale(obj.scale, 0.5f)};
        break;

    case MapObjectType::SPHERE:
        localBox = {{-obj.radius, -obj.radius, -obj.radius}, {obj.radius, obj.radius, obj.radius}};
        break;

    case MapObjectType::CYLINDER:
        localBox = {{-obj.radius, -obj.height / 2, -obj.radius},
                    {obj.radius, obj.height / 2, obj.radius}};
        break;

    case MapObjectType::PLANE:
        localBox = {{-obj.size.x / 2, -0.05f, -obj.size.y / 2},
                    {obj.size.x / 2, 0.05f, obj.size.y / 2}};
        break;

    default:
        localBox = {{-0.5f, -0.5f, -0.5f}, {0.5f, 0.5f, 0.5f}};
        break;
    }

    return GetRayCollisionBox(localRay, localBox);
}

int ViewportPicking::PickObject(ImVec2 mousePos, ImVec2 viewportPos, ImVec2 viewportSize,
                                const Camera3D &camera, const std::shared_ptr<GameScene> &scene)
{
    if (!scene)
        return -1;

    Ray ray = GetMouseRay(mousePos, viewportPos, viewportSize, camera);

    int hitIndex = -1;
    float closestDist = FLT_MAX;

    const auto &objects = scene->GetMapObjects();
    for (size_t i = 0; i < objects.size(); ++i)
    {
        RayCollision collision = TestObjectCollision(ray, objects[i]);
        if (collision.hit && collision.distance < closestDist)
        {
            closestDist = collision.distance;
            hitIndex = static_cast<int>(i);
        }
    }

    return hitIndex;
}

} // namespace CHEngine
