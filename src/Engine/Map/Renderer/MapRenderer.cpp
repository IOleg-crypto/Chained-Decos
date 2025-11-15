#include "MapRenderer.h"
#include "Engine/Render/Utils/RenderUtils.h"
#include <raylib.h>
#include <raymath.h>
#include <filesystem>
#include <algorithm>


void MapRenderer::RenderMap(const GameMap& map, Camera3D camera)
{
    const MapMetadata& metadata = map.GetMapMetaData();
    Skybox* skybox = map.GetSkyBox();
    
    // Set sky color if no skybox, otherwise use skybox
    if (!skybox || !skybox->IsLoaded())
    {
        // Use skyColor if no skybox
        if (metadata.skyColor.a > 0)
        {
            ClearBackground(metadata.skyColor);
        }
        else
        {
            // Use default sky color if no skybox and no skyColor
            ClearBackground(SKYBLUE);
        }
    }

    BeginMode3D(camera);

    // Render skybox first (if present)
    if (skybox && skybox->IsLoaded())
    {
        // Update gamma settings from config before rendering
        //skybox->UpdateGammaFromConfig();
        skybox->DrawSkybox();
    }

    // Render all objects in the map
    for (const auto& object : map.GetMapObjects())
    {
        RenderMapObject(object, map.GetMapModels(), camera);
    }

    EndMode3D();
}

void MapRenderer::RenderMapObject(const MapObjectData& object,
                     const std::unordered_map<std::string, Model>& loadedModels,
                     [[maybe_unused]] Camera3D camera, bool useEditorColors)
{
    // Apply object transformations - ensure consistent order for collision/rendering match
    Matrix translation = MatrixTranslate(object.position.x, object.position.y, object.position.z);
    Matrix scale = MatrixScale(object.scale.x, object.scale.y, object.scale.z);
    Vector3 rotationRad = {object.rotation.x * DEG2RAD, object.rotation.y * DEG2RAD,
                           object.rotation.z * DEG2RAD};
    Matrix rotation = MatrixRotateXYZ(rotationRad);

    // Combine transformations: scale -> rotation -> translation (matches DrawModel/ModelLoader)
    Matrix transform = MatrixMultiply(scale, MatrixMultiply(rotation, translation));

    // For primitives, use direct position and scale (DrawCube, DrawSphere don't use transform
    // matrix) Rotation is not applied to primitives - same as Editor and Game::RenderEditorMap()
    // For models, use transform matrix (DrawModel uses model.transform)
    switch (object.type)
    {
    case MapObjectType::CUBE:
        // DrawCube uses position directly, scale is applied via width/height/length
        DrawCube(object.position, object.scale.x, object.scale.y, object.scale.z, object.color);
        DrawCubeWires(object.position, object.scale.x, object.scale.y, object.scale.z, BLACK);
        break;

    case MapObjectType::SPHERE:
        // DrawSphere uses position directly, radius is from object.radius
        DrawSphere(object.position, object.radius, object.color);
        DrawSphereWires(object.position, object.radius, 16, 16, BLACK);
        break;

    case MapObjectType::CYLINDER:
        // DrawCylinder uses position directly, radius and height from object properties
        DrawCylinder(object.position, object.radius, object.radius, object.height, 16,
                     object.color);
        DrawCylinderWires(object.position, object.radius, object.radius, object.height, 16, BLACK);
        break;

    case MapObjectType::PLANE:
        // DrawPlane uses position directly, size from object.size
        DrawPlane(object.position, Vector2{object.size.x, object.size.y}, object.color);
        break;

    case MapObjectType::SPAWN_ZONE:
        // Spawn zone is rendered separately in game (via MapManager::RenderSpawnZone)
        // Don't render it here as a regular object
        break;

    case MapObjectType::MODEL:
        // Find the corresponding loaded model for this object
        if (!object.modelName.empty())
        {
            // Normalize the lookup key to match how models are stored
            std::string lookupKey = object.modelName;
            std::replace(lookupKey.begin(), lookupKey.end(), '\\', '/');
            std::filesystem::path keyPath(lookupKey);
            std::string keyStem = keyPath.stem().string();
            std::string keyExtension = keyPath.extension().string();
            std::string cleanKey = keyStem;
            if (!keyExtension.empty())
            {
                cleanKey += keyExtension;
            }

            auto it = loadedModels.find(cleanKey);
            if (it != loadedModels.end())
            {
                Model model = it->second;

                // Apply full transform to model (already contains scale, rotation, translation)
                model.transform = transform;

                // Choose tint based on rendering context
                // Editor: use WHITE to preserve textures; Game: use object.color for tinting
                Color tintColor = useEditorColors ? WHITE : object.color;

                // Draw model meshes directly with the full transform matrix
                // This ensures rotation is properly applied (unlike DrawModel which ignores
                // rotation)
                for (int i = 0; i < model.meshCount; i++)
                {
                    // Get material color and apply tint
                    Color color =
                        model.materials[model.meshMaterial[i]].maps[MATERIAL_MAP_DIFFUSE].color;
                    Color colorTint = WHITE;
                    colorTint.r = (unsigned char)(((int)color.r * (int)tintColor.r) / 255);
                    colorTint.g = (unsigned char)(((int)color.g * (int)tintColor.g) / 255);
                    colorTint.b = (unsigned char)(((int)color.b * (int)tintColor.b) / 255);
                    colorTint.a = (unsigned char)(((int)color.a * (int)tintColor.a) / 255);

                    // Apply tint temporarily
                    model.materials[model.meshMaterial[i]].maps[MATERIAL_MAP_DIFFUSE].color =
                        colorTint;

                    // Draw mesh with full transform
                    DrawMesh(model.meshes[i], model.materials[model.meshMaterial[i]], transform);

                    // Restore original color
                    model.materials[model.meshMaterial[i]].maps[MATERIAL_MAP_DIFFUSE].color = color;
                }

                // Optional: Draw model wires for debugging (disabled in runtime)
                // DrawModelWires(model, Vector3{0, 0, 0}, 1.0f, BLACK);
            }
            else
            {
                // Try fallback lookup with original modelName
                auto it2 = loadedModels.find(object.modelName);
                if (it2 != loadedModels.end())
                {
                    Model model = it2->second;
                    model.transform = transform;

                    // Choose tint based on rendering context
                    Color tintColor = useEditorColors ? WHITE : object.color;

                    // Draw model meshes directly with the full transform matrix
                    for (int i = 0; i < model.meshCount; i++)
                    {
                        // Get material color and apply tint
                        Color color =
                            model.materials[model.meshMaterial[i]].maps[MATERIAL_MAP_DIFFUSE].color;
                        Color colorTint = WHITE;
                        colorTint.r = (unsigned char)(((int)color.r * (int)tintColor.r) / 255);
                        colorTint.g = (unsigned char)(((int)color.g * (int)tintColor.g) / 255);
                        colorTint.b = (unsigned char)(((int)color.b * (int)tintColor.b) / 255);
                        colorTint.a = (unsigned char)(((int)color.a * (int)tintColor.a) / 255);

                        // Apply tint temporarily
                        model.materials[model.meshMaterial[i]].maps[MATERIAL_MAP_DIFFUSE].color =
                            colorTint;

                        // Draw mesh with full transform
                        DrawMesh(model.meshes[i], model.materials[model.meshMaterial[i]],
                                 transform);

                        // Restore original color
                        model.materials[model.meshMaterial[i]].maps[MATERIAL_MAP_DIFFUSE].color = color;
                    }
                    // DrawModelWires(model, Vector3{0, 0, 0}, 1.0f, BLACK);
                }
                else
                {
                    // Model not found, draw placeholder
                    DrawSphere(object.position, 0.5f, RED);
                    TraceLog(LOG_WARNING,
                             "RenderMapObject: Model not found for %s (tried keys: %s, %s)",
                             object.name.c_str(), cleanKey.c_str(), object.modelName.c_str());
                }
            }
        }
        else
        {
            // No model name specified, draw placeholder
            DrawSphere(object.position, 0.5f, RED);
        }
        break;

    case MapObjectType::LIGHT:
        // Light objects don't render visually, they affect lighting
        DrawSphere(object.position, 0.2f, YELLOW); // Visual representation of light
        break;

    default:
        // Unknown object type - draw as cube
        DrawCube(object.position, object.scale.x, object.scale.y, object.scale.z, object.color);
        DrawCubeWires(object.position, object.scale.x, object.scale.y, object.scale.z, BLACK);
        break;
    }
}

void MapRenderer::RenderSpawnZone(Texture2D spawnTexture, const Vector3& position, float size, Color color, bool textureLoaded) const
{
    RenderSpawnZoneWithTexture(spawnTexture, position, size, color, textureLoaded);
}

void MapRenderer::RenderSpawnZoneWithTexture(Texture2D texture, const Vector3& position, float size, Color color, bool textureLoaded) const
{
    if (!textureLoaded)
    {
        // Fallback to simple cube if texture not loaded
        DrawCube(position, size, size, size, color);
        DrawCubeWires(position, size, size, size, WHITE);
        return;
    }

    // Use shared RenderUtils function to draw textured cube
    RenderUtils::DrawCubeTexture(texture, position, size, size, size, color);

    // Draw wireframe for better visibility
    DrawCubeWires(position, size, size, size, WHITE);
}

