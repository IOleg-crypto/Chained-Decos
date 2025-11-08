#include "MapObject.h"

MapObject::MapObject()
    : position{0, 0, 0}, scale{1, 1, 1}, rotation{0, 0, 0}, color{WHITE}, objectName(""),
      objectType(0), isSelected(false), planeSize{2, 2}, horizontalRadius(1), verticalRadius(1),
      sphereRadius(1), modelAssetName("")
{
}

MapObject::~MapObject() = default;

// Getters
Vector3 MapObject::GetPosition() const
{
    return position;
}
Vector3 MapObject::GetScale() const
{
    return scale;
}
Vector3 MapObject::GetRotation() const
{
    return rotation;
}
Color MapObject::GetColor() const
{
    return color;
}
std::string MapObject::GetObjectName() const
{
    return objectName;
}
int MapObject::GetObjectType() const
{
    return objectType;
}
bool MapObject::IsSelected() const
{
    return isSelected;
}
Vector2 MapObject::GetPlaneSize() const
{
    return planeSize;
}
float MapObject::GetVerticalRadius() const
{
    return verticalRadius;
}
float MapObject::GetHorizontalRadius() const
{
    return horizontalRadius;
}
float MapObject::GetSphereRadius() const
{
    return sphereRadius;
}
std::string MapObject::GetModelAssetName() const
{
    return modelAssetName;
}

// Setters
void MapObject::SetPosition(const Vector3 &pos)
{
    position = pos;
}
void MapObject::SetScale(const Vector3 &scale)
{
    this->scale = scale;
}
void MapObject::SetRotation(const Vector3 &rot)
{
    rotation = rot;
}
void MapObject::SetColor(const Color &col)
{
    color = col;
}
void MapObject::SetObjectName(const std::string &objectName)
{
    this->objectName = objectName;
}
void MapObject::SetObjectType(int objectType)
{
    this->objectType = objectType;
}
void MapObject::SetSelected(bool isSelected)
{
    this->isSelected = isSelected;
}
void MapObject::SetPlaneSize(const Vector2 &planeSize)
{
    this->planeSize = planeSize;
}
void MapObject::SetVerticalRadius(float verticalRadius)
{
    this->verticalRadius = verticalRadius;
}
void MapObject::SetHorizontalRadius(float horizontalRadius)
{
    this->horizontalRadius = horizontalRadius;
}
void MapObject::SetSphereRadius(float sphereRadius)
{
    this->sphereRadius = sphereRadius;
}
void MapObject::SetModelAssetName(const std::string &modelAssetName)
{
    this->modelAssetName = modelAssetName;
}