#ifndef BOUNDINGBOXCALCULATORS_H
#define BOUNDINGBOXCALCULATORS_H

#include "IBoundingBoxCalculator.h"
#include <memory>

// Cube bounding box calculator
class CubeBoundingBoxCalculator : public IBoundingBoxCalculator {
public:
    BoundingBox CalculateBoundingBox(const MapObject& obj) const override;
};

// Sphere bounding box calculator
class SphereBoundingBoxCalculator : public IBoundingBoxCalculator {
public:
    BoundingBox CalculateBoundingBox(const MapObject& obj) const override;
};

// Cylinder bounding box calculator
class CylinderBoundingBoxCalculator : public IBoundingBoxCalculator {
public:
    BoundingBox CalculateBoundingBox(const MapObject& obj) const override;
};

// Plane bounding box calculator
class PlaneBoundingBoxCalculator : public IBoundingBoxCalculator {
public:
    BoundingBox CalculateBoundingBox(const MapObject& obj) const override;
};

// Ellipse bounding box calculator
class EllipseBoundingBoxCalculator : public IBoundingBoxCalculator {
public:
    BoundingBox CalculateBoundingBox(const MapObject& obj) const override;
};

// Model bounding box calculator
class ModelBoundingBoxCalculator : public IBoundingBoxCalculator {
public:
    BoundingBox CalculateBoundingBox(const MapObject& obj) const override;
};

// Factory for creating appropriate bounding box calculators
class BoundingBoxCalculatorFactory {
public:
    static std::unique_ptr<IBoundingBoxCalculator> CreateCalculator(int objectType);
};

#endif // BOUNDINGBOXCALCULATORS_H

