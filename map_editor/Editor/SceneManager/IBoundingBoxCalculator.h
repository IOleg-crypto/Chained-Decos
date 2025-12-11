#ifndef IBOUNDINGBOXCALCULATOR_H
#define IBOUNDINGBOXCALCULATOR_H

#include <raylib.h>
#include "../Object/MapObject.h"

// Strategy interface for calculating bounding boxes for different object types
class IBoundingBoxCalculator {
public:
    virtual ~IBoundingBoxCalculator() = default;
    
    // Calculate bounding box for the given object
    virtual BoundingBox CalculateBoundingBox(const MapObject& obj) const = 0;
};

#endif // IBOUNDINGBOXCALCULATOR_H

