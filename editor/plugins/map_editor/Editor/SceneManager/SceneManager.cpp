#include "SceneManager.h"
#include "../Object/MapObject.h"
#include "servers/physics/collision/Structures/CollisionStructures.h"
#include "BoundingBoxCalculators.h"
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

        // Use Strategy pattern to calculate bounding box based on object type
        auto calculator = BoundingBoxCalculatorFactory::CreateCalculator(obj.GetObjectType());
        BoundingBox box = calculator->CalculateBoundingBox(obj);

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