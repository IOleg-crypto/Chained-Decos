#include "MapObject.h"

MapObject::MapObject()
    : position{0, 0, 0}, scale{1, 1, 1}, rotation{0, 0, 0}, color{WHITE}, name(""), type(0),
      selected(false), size{2, 2}, radiusH(1), radiusV(1), radiusSphere(1), modelName("")
{
}

MapObject::~MapObject() = default;

// Getters
Vector3 MapObject::GetPosition() const { return position; }
Vector3 MapObject::GetScale() const { return scale; }
Vector3 MapObject::GetRotation() const { return rotation; }
Color MapObject::GetColor() const { return color; }
std::string MapObject::GetName() const { return name; }
int &MapObject::GetType() { return type; }
bool MapObject::IsSelected() const { return selected; }
Vector2 MapObject::GetSize() const { return size; }
float MapObject::GetRadiusV() const { return radiusV; }
float MapObject::GetRadiusH() const { return radiusH; }
float MapObject::GetRadiusSphere() const { return radiusSphere; }
bool MapObject::GetSelected() const { return selected; }
std::string MapObject::GetModelName() const { return modelName; }

// Setters
void MapObject::SetPosition(const Vector3 &pos) { position = pos; }
void MapObject::SetScale(const Vector3 &scale) { this->scale = scale; }
void MapObject::SetRotation(const Vector3 &rot) { rotation = rot; }
void MapObject::SetColor(const Color &col) { color = col; }
void MapObject::SetName(const std::string &name) { this->name = name; }
void MapObject::SetType(int type) { this->type = type; }
void MapObject::SetSelected(bool selected) { this->selected = selected; }
void MapObject::SetSize(const Vector2 &size) { this->size = size; }
void MapObject::SetRadiusV(float radiusV) { this->radiusV = radiusV; }
void MapObject::SetRadiusH(float radiusH) { this->radiusH = radiusH; }
void MapObject::SetRadiusSphere(float radiusSphere) { this->radiusSphere = radiusSphere; }
void MapObject::SetModelName(const std::string &modelName) { this->modelName = modelName; }