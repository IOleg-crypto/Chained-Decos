#include "MapRenderer.h"
#include "components/rendering/utils/RenderUtils.h"
#include <algorithm>
#include <filesystem>
#include <raylib.h>
#include <raymath.h>

void MapRenderer::RenderMap(const GameMap &map, Camera3D camera)
{
    const MapMetadata &metadata = map.GetMapMetaData();
    Skybox *skybox = map.GetSkyBox();

    // Set sky color if no skybox, otherwise use skybox
    if (!skybox || !skybox->IsLoaded())
    {
        if (metadata.skyColor.a > 0)
        {
            ClearBackground(metadata.skyColor);
        }
        else
        {
            ClearBackground(SKYBLUE);
        }
    }

    BeginMode3D(camera);

    if (skybox && skybox->IsLoaded())
    {
        skybox->DrawSkybox(camera.position);
    }

    // Render all objects in the map
    for (const auto &object : map.GetMapObjects())
    {
        RenderMapObject(object, map.GetMapModels(), camera);
    }

    EndMode3D();
}

void MapRenderer::RenderMapObject(const MapObjectData &object,
                                  const std::unordered_map<std::string, Model> &loadedModels,
                                  [[maybe_unused]] Camera3D camera, bool useEditorColors,
                                  bool wireframe)
{
    // Apply object transformations - ensure consistent order for collision/rendering match
    Matrix translation = MatrixTranslate(object.position.x, object.position.y, object.position.z);
    Matrix scale = MatrixScale(object.scale.x, object.scale.y, object.scale.z);
    Vector3 rotationRad = {object.rotation.x * DEG2RAD, object.rotation.y * DEG2RAD,
                           object.rotation.z * DEG2RAD};
    Matrix rotation = MatrixRotateXYZ(rotationRad);

    // Combine transformations: T * R * S (Correct world matrix order)
    Matrix transform = MatrixMultiply(MatrixMultiply(translation, rotation), scale);

    switch (object.type)
    {
    case MapObjectType::CUBE:
        if (wireframe)
            DrawCubeWires(object.position, object.scale.x, object.scale.y, object.scale.z,
                          object.color);
        else
            DrawCube(object.position, object.scale.x, object.scale.y, object.scale.z, object.color);
        break;

    case MapObjectType::SPHERE:
        if (wireframe)
            DrawSphereWires(object.position, object.radius, 16, 16, object.color);
        else
            DrawSphere(object.position, object.radius, object.color);
        break;

    case MapObjectType::CYLINDER:
        if (wireframe)
            DrawCylinderWires(object.position, object.radius, object.radius, object.height, 16,
                              object.color);
        else
            DrawCylinder(object.position, object.radius, object.radius, object.height, 16,
                         object.color);
        break;

    case MapObjectType::PLANE:
        if (wireframe)
        {
            Vector3 p1 = {object.position.x - object.size.x * 0.5f, object.position.y,
                          object.position.z - object.size.y * 0.5f};
            Vector3 p2 = {object.position.x + object.size.x * 0.5f, object.position.y,
                          object.position.z - object.size.y * 0.5f};
            Vector3 p3 = {object.position.x + object.size.x * 0.5f, object.position.y,
                          object.position.z + object.size.y * 0.5f};
            Vector3 p4 = {object.position.x - object.size.x * 0.5f, object.position.y,
                          object.position.z + object.size.y * 0.5f};
            DrawLine3D(p1, p2, object.color);
            DrawLine3D(p2, p3, object.color);
            DrawLine3D(p3, p4, object.color);
            DrawLine3D(p4, p1, object.color);
        }
        else
        {
            DrawPlane(object.position, Vector2{object.size.x, object.size.y}, object.color);
        }
        break;

    case MapObjectType::MODEL:
        if (!object.modelName.empty())
        {
            std::string lookupKey = object.modelName;
            std::replace(lookupKey.begin(), lookupKey.end(), '\\', '/');
            std::filesystem::path keyPath(lookupKey);
            std::string cleanKey = keyPath.filename().string();

            auto it = loadedModels.find(cleanKey);
            if (it == loadedModels.end())
                it = loadedModels.find(object.modelName);

            if (it != loadedModels.end())
            {
                Model model = it->second;
                model.transform = transform;
                Color tintColor = useEditorColors ? WHITE : object.color;

                if (wireframe)
                {
                    DrawModelWires(model, {0, 0, 0}, 1.0f, tintColor);
                }
                else
                {
                    for (int i = 0; i < model.meshCount; i++)
                    {
                        Color color =
                            model.materials[model.meshMaterial[i]].maps[MATERIAL_MAP_DIFFUSE].color;
                        Color colorTint = WHITE;
                        colorTint.r = (unsigned char)(((int)color.r * (int)tintColor.r) / 255);
                        colorTint.g = (unsigned char)(((int)color.g * (int)tintColor.g) / 255);
                        colorTint.b = (unsigned char)(((int)color.b * (int)tintColor.b) / 255);
                        colorTint.a = (unsigned char)(((int)color.a * (int)tintColor.a) / 255);

                        model.materials[model.meshMaterial[i]].maps[MATERIAL_MAP_DIFFUSE].color =
                            colorTint;
                        DrawMesh(model.meshes[i], model.materials[model.meshMaterial[i]],
                                 transform);
                        model.materials[model.meshMaterial[i]].maps[MATERIAL_MAP_DIFFUSE].color =
                            color;
                    }
                }
            }
            else
            {
                DrawSphere(object.position, 0.5f, RED);
            }
        }
        break;

    case MapObjectType::SPAWN_ZONE:
        break;

    default:
        DrawCube(object.position, object.scale.x, object.scale.y, object.scale.z, object.color);
        break;
    }
}

void MapRenderer::RenderSpawnZone(Texture2D spawnTexture, const Vector3 &position, float size,
                                  Color color, bool textureLoaded) const
{
    RenderSpawnZoneWithTexture(spawnTexture, position, size, color, textureLoaded);
}

void MapRenderer::RenderSpawnZoneWithTexture(Texture2D texture, const Vector3 &position, float size,
                                             Color color, bool textureLoaded) const
{
    if (!textureLoaded)
    {
        DrawCube(position, size, size, size, color);
        DrawCubeWires(position, size, size, size, WHITE);
        return;
    }
    RenderUtils::DrawCubeTexture(texture, position, size, size, size, color);
    DrawCubeWires(position, size, size, size, WHITE);
}
