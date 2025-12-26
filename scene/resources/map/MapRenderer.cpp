#include "MapRenderer.h"
#include "components/rendering/utils/RenderUtils.h"
#include <algorithm>
#include <filesystem>
#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>

void MapRenderer::RenderMap(const GameScene &map, Camera3D camera)
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
    DrawMapContent(map, camera);
    EndMode3D();
}

void MapRenderer::DrawMapContent(const GameScene &map, Camera3D camera)
{
    const MapMetadata &metadata = map.GetMapMetaData();
    Skybox *skybox = map.GetSkyBox();

    if (skybox && skybox->IsLoaded())
    {
        skybox->DrawSkybox(camera.position);
    }

    // Render all objects in the map
    for (const auto &object : map.GetMapObjects())
    {
        RenderMapObject(object, map.GetMapModels(), camera);
    }
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

    Color drawColor = (useEditorColors && wireframe) ? YELLOW : object.color;

    // Combine transformations: T * R * S (Correct world matrix order)
    Matrix transform = MatrixMultiply(MatrixMultiply(translation, rotation), scale);

    switch (object.type)
    {
    case MapObjectType::CUBE:
    {
        rlPushMatrix();
        rlMultMatrixf(MatrixToFloat(transform));
        // Cubes use scale as their primary dimensions
        if (wireframe)
            DrawCubeWires({0, 0, 0}, 1.0f, 1.0f, 1.0f, drawColor);
        else
            DrawCube({0, 0, 0}, 1.0f, 1.0f, 1.0f, drawColor);
        rlPopMatrix();
    }
    break;

    case MapObjectType::SPHERE:
    {
        rlPushMatrix();
        rlMultMatrixf(MatrixToFloat(transform));
        if (wireframe)
            DrawSphereWires({0, 0, 0}, object.radius, 16, 16, drawColor);
        else
            DrawSphere({0, 0, 0}, object.radius, drawColor);
        rlPopMatrix();
    }
    break;

    case MapObjectType::CYLINDER:
    {
        rlPushMatrix();
        rlMultMatrixf(MatrixToFloat(transform));
        if (wireframe)
            DrawCylinderWires({0, 0, 0}, object.radius, object.radius, object.height, 16,
                              drawColor);
        else
            DrawCylinder({0, 0, 0}, object.radius, object.radius, object.height, 16, drawColor);
        rlPopMatrix();
    }
    break;

    case MapObjectType::PLANE:
    {
        rlPushMatrix();
        rlMultMatrixf(MatrixToFloat(transform));
        if (wireframe)
        {
            Vector3 p1 = {-object.size.x * 0.5f, 0.0f, -object.size.y * 0.5f};
            Vector3 p2 = {object.size.x * 0.5f, 0.0f, -object.size.y * 0.5f};
            Vector3 p3 = {object.size.x * 0.5f, 0.0f, object.size.y * 0.5f};
            Vector3 p4 = {-object.size.x * 0.5f, 0.0f, object.size.y * 0.5f};
            DrawLine3D(p1, p2, drawColor);
            DrawLine3D(p2, p3, drawColor);
            DrawLine3D(p3, p4, drawColor);
            DrawLine3D(p4, p1, drawColor);
        }
        else
        {
            DrawPlane({0, 0, 0}, object.size, drawColor);
        }
        rlPopMatrix();
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

    case MapObjectType::LIGHT:
    {
        rlPushMatrix();
        rlMultMatrixf(MatrixToFloat(transform));
        if (wireframe)
            DrawSphereWires({0, 0, 0}, 0.5f, 8, 8, drawColor);
        else
            DrawSphere({0, 0, 0}, 0.5f, drawColor);
        rlPopMatrix();
    }
    break;

    case MapObjectType::SPAWN_ZONE:
    {
        rlPushMatrix();
        rlMultMatrixf(MatrixToFloat(transform));

        // Draw a translucent cyan box to represent the spawn zone
        Color spawnColor = {0, 255, 255, 100}; // Translucent Cyan
        if (!wireframe)
            DrawCube({0, 0, 0}, 1.0f, 1.0f, 1.0f, spawnColor);

        DrawCubeWires({0, 0, 0}, 1.0f, 1.0f, 1.0f, drawColor);

        // Draw an arrow or marker to show forward direction (Y rotation)
        // Since we are already in the matrix stack, forward is just {0, 0, 1}
        Vector3 forwardEnd = {0, 0, 1.0f};
        DrawLine3D({0, 0, 0}, forwardEnd, drawColor);
        DrawSphere(forwardEnd, 0.1f, drawColor);

        rlPopMatrix();
    }
    break;

    default:
    {
        rlPushMatrix();
        rlMultMatrixf(MatrixToFloat(transform));
        if (wireframe)
            DrawCubeWires({0, 0, 0}, 1.0f, 1.0f, 1.0f, drawColor);
        else
            DrawCube({0, 0, 0}, 1.0f, 1.0f, 1.0f, drawColor);
        rlPopMatrix();
    }
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
