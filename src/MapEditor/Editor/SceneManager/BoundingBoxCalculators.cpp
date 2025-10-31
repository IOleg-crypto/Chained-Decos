#include "BoundingBoxCalculators.h"
#include <memory>

// Cube bounding box implementation
BoundingBox CubeBoundingBoxCalculator::CalculateBoundingBox(const MapObject& obj) const {
    Vector3 position = obj.GetPosition();
    Vector3 scale = obj.GetScale();
    
    return {
        Vector3{position.x - scale.x, position.y - scale.y, position.z - scale.z},
        Vector3{position.x + scale.x, position.y + scale.y, position.z + scale.z}
    };
}

// Sphere bounding box implementation
BoundingBox SphereBoundingBoxCalculator::CalculateBoundingBox(const MapObject& obj) const {
    Vector3 position = obj.GetPosition();
    float radius = obj.GetSphereRadius() * obj.GetScale().x;
    
    return {
        Vector3{position.x - radius, position.y - radius, position.z - radius},
        Vector3{position.x + radius, position.y + radius, position.z + radius}
    };
}

// Cylinder bounding box implementation
BoundingBox CylinderBoundingBoxCalculator::CalculateBoundingBox(const MapObject& obj) const {
    Vector3 position = obj.GetPosition();
    Vector3 scale = obj.GetScale();
    
    return {
        Vector3{position.x - scale.x * 0.5f, position.y - scale.y, position.z - scale.x * 0.5f},
        Vector3{position.x + scale.x * 0.5f, position.y + scale.y, position.z + scale.x * 0.5f}
    };
}

// Plane bounding box implementation
BoundingBox PlaneBoundingBoxCalculator::CalculateBoundingBox(const MapObject& obj) const {
    Vector3 position = obj.GetPosition();
    Vector3 scale = obj.GetScale();
    Vector2 planeSize = obj.GetPlaneSize();
    
    return {
        Vector3{position.x - planeSize.x * scale.x, position.y - 0.1f, position.z - planeSize.y * scale.z},
        Vector3{position.x + planeSize.x * scale.x, position.y + 0.1f, position.z + planeSize.y * scale.z}
    };
}

// Ellipse bounding box implementation
BoundingBox EllipseBoundingBoxCalculator::CalculateBoundingBox(const MapObject& obj) const {
    Vector3 position = obj.GetPosition();
    Vector3 scale = obj.GetScale();
    float radiusX = obj.GetHorizontalRadius() * scale.x;
    float radiusZ = obj.GetVerticalRadius() * scale.z;
    
    return {
        Vector3{position.x - radiusX, position.y - 0.5f * scale.y, position.z - radiusZ},
        Vector3{position.x + radiusX, position.y + 0.5f * scale.y, position.z + radiusZ}
    };
}

// Model bounding box implementation
BoundingBox ModelBoundingBoxCalculator::CalculateBoundingBox(const MapObject& obj) const {
    Vector3 position = obj.GetPosition();
    Vector3 scale = obj.GetScale();
    
    // For models, use a default bounding box for now
    // TODO: In future, get actual model bounds from ModelLoader
    return {
        Vector3{position.x - scale.x, position.y - scale.y, position.z - scale.z},
        Vector3{position.x + scale.x, position.y + scale.y, position.z + scale.z}
    };
}

// Factory implementation
std::unique_ptr<IBoundingBoxCalculator> BoundingBoxCalculatorFactory::CreateCalculator(int objectType) {
    switch (objectType) {
        case 0: // Cube
            return std::make_unique<CubeBoundingBoxCalculator>();
        case 1: // Sphere
            return std::make_unique<SphereBoundingBoxCalculator>();
        case 2: // Cylinder
            return std::make_unique<CylinderBoundingBoxCalculator>();
        case 3: // Plane
            return std::make_unique<PlaneBoundingBoxCalculator>();
        case 4: // Ellipse
            return std::make_unique<EllipseBoundingBoxCalculator>();
        case 5: // Model
            return std::make_unique<ModelBoundingBoxCalculator>();
        default:
            // Unknown type - use default cube bounds
            return std::make_unique<CubeBoundingBoxCalculator>();
    }
}

