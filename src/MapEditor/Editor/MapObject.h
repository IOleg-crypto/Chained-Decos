#ifndef MAPOBJECT_H
#define MAPOBJECT_H

#include <raylib.h>
#include <string>

// Structure for map objects (cubes, spheres, cylinders , etc)
struct MapObject
{
    Vector3 position; // Object position in 3D space
    Vector3 scale;    // Object scale (size)
    Vector3 rotation; // Object rotation in radians
    Color color;      // Object color
    std::string name; // Object name for identification
    int type;         // Object type: 0=cube, 1=sphere, 2=cylinder
    bool selected;    // Selection state for UI
    Vector2 size;     // If it`s plane or other meshes
    // For ellipse
    float radiusV;
    float radiusH;
    float radiusSphere;

    MapObject();
};

#endif