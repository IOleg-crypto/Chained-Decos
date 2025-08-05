#include "MapObject.h"

MapObject::MapObject()
    : position{0, 0, 0}, scale{1, 1, 1}, rotation{0, 0, 0}, color{WHITE}, name(""), type(0),
      selected(false), size{2, 2}, radiusH(1), radiusV(1), radiusSphere(1)
{
}