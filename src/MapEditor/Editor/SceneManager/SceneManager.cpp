#include "SceneManager.h"
#include "../Object/MapObject.h"
#include "Engine/Collision/CollisionStructures.h"
#include <algorithm>
#include <limits>
#include <raylib.h>

SceneManager::SceneManager() : m_selectedIndex(-1) {}

void SceneManager::AddObject(const MapObject& obj) {
    m_objects.push_back(obj);
}

void SceneManager::RemoveObject(int index) {
    if (IsValidIndex(index)) {
        m_objects.erase(m_objects.begin() + index);
        UpdateSelectionIndices(index);
    }
}

void SceneManager::SelectObject(int index) {
    // Clear previous selection
    if (IsValidIndex(m_selectedIndex)) {
        m_objects[m_selectedIndex].SetSelected(false);
    }

    m_selectedIndex = index;

    // Set new selection
    if (IsValidIndex(index)) {
        m_objects[index].SetSelected(true);
    }
}

void SceneManager::ClearSelection() {
    if (IsValidIndex(m_selectedIndex)) {
        m_objects[m_selectedIndex].SetSelected(false);
    }
    m_selectedIndex = -1;
}

MapObject* SceneManager::GetSelectedObject() {
    if (IsValidIndex(m_selectedIndex)) {
        return &m_objects[m_selectedIndex];
    }
    return nullptr;
}

const std::vector<MapObject>& SceneManager::GetObjects() const {
    return m_objects;
}

int SceneManager::GetSelectedObjectIndex() const {
    return m_selectedIndex;
}

int SceneManager::PickObject(const CollisionRay& ray) {
    int pickedIndex = -1;
    float minDistance = std::numeric_limits<float>::infinity();

    // Convert CollisionRay to Raylib Ray
    Ray rlRay = {ray.GetOrigin(), ray.GetDirection()};

    for (size_t i = 0; i < m_objects.size(); ++i) {
        const auto& obj = m_objects[i];

        // Create bounding box for the object based on type
        Vector3 position = obj.GetPosition();
        Vector3 scale = obj.GetScale();
        BoundingBox box = {};

        switch (obj.GetObjectType()) {
            case 0: // Cube
                box = {
                    Vector3{position.x - scale.x, position.y - scale.y, position.z - scale.z},
                    Vector3{position.x + scale.x, position.y + scale.y, position.z + scale.z}
                };
                break;
            case 1: // Sphere
                {
                    float radius = obj.GetSphereRadius() * scale.x;
                    box = {
                        Vector3{position.x - radius, position.y - radius, position.z - radius},
                        Vector3{position.x + radius, position.y + radius, position.z + radius}
                    };
                }
                break;
            case 2: // Cylinder
                box = {
                    Vector3{position.x - scale.x * 0.5f, position.y - scale.y, position.z - scale.x * 0.5f},
                    Vector3{position.x + scale.x * 0.5f, position.y + scale.y, position.z + scale.x * 0.5f}
                };
                break;
            case 3: // Plane
                {
                    Vector2 planeSize = obj.GetPlaneSize();
                    box = {
                        Vector3{position.x - planeSize.x * scale.x, position.y - 0.1f, position.z - planeSize.y * scale.z},
                        Vector3{position.x + planeSize.x * scale.x, position.y + 0.1f, position.z + planeSize.y * scale.z}
                    };
                }
                break;
            case 4: // Ellipse
                {
                    float radiusX = obj.GetHorizontalRadius() * scale.x;
                    float radiusZ = obj.GetVerticalRadius() * scale.z;
                    box = {
                        Vector3{position.x - radiusX, position.y - 0.5f * scale.y, position.z - radiusZ},
                        Vector3{position.x + radiusX, position.y + 0.5f * scale.y, position.z + radiusZ}
                    };
                }
                break;
            case 5: // Model
                // For models, use a default bounding box for now
                // TODO: In future, get actual model bounds from ModelLoader
                box = {
                    Vector3{position.x - scale.x, position.y - scale.y, position.z - scale.z},
                    Vector3{position.x + scale.x, position.y + scale.y, position.z + scale.z}
                };
                break;
            default:
                // Unknown type - use default cube bounds
                box = {
                    Vector3{position.x - scale.x, position.y - scale.y, position.z - scale.z},
                    Vector3{position.x + scale.x, position.y + scale.y, position.z + scale.z}
                };
                break;
        }

        // Check ray intersection with bounding box
        RayCollision collision = GetRayCollisionBox(rlRay, box);

        if (collision.hit && collision.distance < minDistance) {
            minDistance = collision.distance;
            pickedIndex = static_cast<int>(i);
        }
    }

    // Update selection state
    if (pickedIndex != -1) {
        SelectObject(pickedIndex);
    } else {
        ClearSelection();
    }

    return pickedIndex;
}

bool SceneManager::IsValidIndex(int index) const {
    return index >= 0 && index < static_cast<int>(m_objects.size());
}

void SceneManager::UpdateSelectionIndices(int removedIndex) {
    if (m_selectedIndex == removedIndex) {
        // Selected object was removed
        m_selectedIndex = -1;
    } else if (m_selectedIndex > removedIndex) {
        // Selected object index needs to be decremented
        m_selectedIndex--;
    }
}