#include "MeasurementTools.h"
#include <raylib.h>
#include <raymath.h>
#include <algorithm>
#include <cmath>
#include <iostream>

MeasurementTools::MeasurementTools()
    : m_activeMeasurement(-1)
    , m_measurementMode(false)
    , m_snappingEnabled(true)
    , m_gridVisible(true)
    , m_gridOrigin({0.0f, 0.0f, 0.0f})
    , m_gridSize({10.0f, 10.0f, 10.0f})
    , m_gridColor({128, 128, 128, 128})
    , m_rulerActive(false)
    , m_protractorActive(false)
    , m_areaMeasurementActive(false)
{
}

void MeasurementTools::Initialize()
{
    // Initialize default settings
    m_snapSettings.gridSize = 1.0f;
    m_snapSettings.snapTolerance = 0.1f;
    m_snapSettings.snapRotation = true;
    m_snapSettings.rotationAngle = 15.0f;
}

void MeasurementTools::Reset()
{
    ClearAllMeasurements();
    m_activeMeasurement = -1;
    m_measurementMode = false;
    m_rulerActive = false;
    m_protractorActive = false;
    m_areaMeasurementActive = false;
}

int MeasurementTools::StartMeasurement(MeasurementType type, const std::string& name)
{
    Measurement measurement;
    measurement.type = type;
    measurement.name = name.empty() ? "Measurement_" + std::to_string(m_measurements.size()) : name;
    measurement.color = (type == MeasurementType::DISTANCE) ? YELLOW :
                       (type == MeasurementType::ANGLE) ? GREEN :
                       (type == MeasurementType::AREA) ? BLUE :
                       (type == MeasurementType::VOLUME) ? PURPLE : WHITE;

    m_measurements.push_back(measurement);
    m_activeMeasurement = static_cast<int>(m_measurements.size()) - 1;
    m_measurementMode = true;

    return m_activeMeasurement;
}

bool MeasurementTools::AddMeasurementPoint(int measurementIndex, const Vector3& point)
{
    if (measurementIndex < 0 || measurementIndex >= static_cast<int>(m_measurements.size()))
        return false;

    m_measurements[measurementIndex].points.push_back(point);
    UpdateMeasurement(measurementIndex);

    return true;
}

bool MeasurementTools::CompleteMeasurement(int measurementIndex)
{
    if (measurementIndex < 0 || measurementIndex >= static_cast<int>(m_measurements.size()))
        return false;

    m_measurementMode = false;
    return true;
}

bool MeasurementTools::DeleteMeasurement(int index)
{
    if (index < 0 || index >= static_cast<int>(m_measurements.size()))
        return false;

    m_measurements.erase(m_measurements.begin() + index);

    if (m_activeMeasurement >= static_cast<int>(m_measurements.size()))
    {
        m_activeMeasurement = static_cast<int>(m_measurements.size()) - 1;
    }

    return true;
}

void MeasurementTools::ClearAllMeasurements()
{
    m_measurements.clear();
    m_activeMeasurement = -1;
}

float MeasurementTools::GetMeasurementValue(int index) const
{
    if (index < 0 || index >= static_cast<int>(m_measurements.size()))
        return 0.0f;

    return m_measurements[index].value;
}

std::string MeasurementTools::GetMeasurementInfo(int index) const
{
    if (index < 0 || index >= static_cast<int>(m_measurements.size()))
        return "Invalid Measurement";

    const Measurement& measurement = m_measurements[index];
    std::string info = measurement.name + ": ";

    switch (measurement.type)
    {
        case MeasurementType::DISTANCE:
            info += std::to_string(measurement.value) + " units";
            break;
        case MeasurementType::ANGLE:
            info += std::to_string(measurement.value) + " degrees";
            break;
        case MeasurementType::AREA:
            info += std::to_string(measurement.value) + " sq units";
            break;
        case MeasurementType::VOLUME:
            info += std::to_string(measurement.value) + " cu units";
            break;
        case MeasurementType::COORDINATES:
            info += "Position data";
            break;
    }

    return info;
}

Vector3 MeasurementTools::GetMeasurementPoint(int measurementIndex, int pointIndex) const
{
    if (measurementIndex < 0 || measurementIndex >= static_cast<int>(m_measurements.size()) ||
        pointIndex < 0 || pointIndex >= static_cast<int>(m_measurements[measurementIndex].points.size()))
    {
        return {0.0f, 0.0f, 0.0f};
    }

    return m_measurements[measurementIndex].points[pointIndex];
}

void MeasurementTools::SetActiveMeasurement(int index)
{
    if (index >= -1 && index < static_cast<int>(m_measurements.size()))
    {
        m_activeMeasurement = index;
    }
}

Vector3 MeasurementTools::SnapPoint(const Vector3& point, const std::vector<MapObject>& objects)
{
    if (!m_snappingEnabled)
        return point;

    return FindSnapPoint(point, objects);
}

Vector3 MeasurementTools::SnapRotation(const Vector3& rotation)
{
    if (!m_snappingEnabled || !m_snapSettings.snapRotation)
        return rotation;

    Vector3 snapped = rotation;
    float angle = m_snapSettings.rotationAngle * DEG2RAD;

    snapped.x = roundf(snapped.x / angle) * angle;
    snapped.y = roundf(snapped.y / angle) * angle;
    snapped.z = roundf(snapped.z / angle) * angle;

    return snapped;
}

void MeasurementTools::EnableSnapping(bool enable)
{
    m_snappingEnabled = enable;
}

void MeasurementTools::SetSnapMode(SnapMode mode)
{
    m_snapSettings.mode = mode;
}

void MeasurementTools::SetGridSize(float size)
{
    m_snapSettings.gridSize = std::max(0.01f, size);
}

void MeasurementTools::SetSnapTolerance(float tolerance)
{
    m_snapSettings.snapTolerance = std::max(0.01f, tolerance);
}

void MeasurementTools::SetSnapToVertices(bool enable)
{
    m_snapSettings.snapToVertices = enable;
}

void MeasurementTools::SetSnapToEdges(bool enable)
{
    m_snapSettings.snapToEdges = enable;
}

void MeasurementTools::SetSnapToFaces(bool enable)
{
    m_snapSettings.snapToFaces = enable;
}

void MeasurementTools::SetSnapToObjectCenters(bool enable)
{
    m_snapSettings.snapToObjectCenters = enable;
}

void MeasurementTools::SetSnapRotation(bool enable)
{
    m_snapSettings.snapRotation = enable;
}

void MeasurementTools::SetRotationAngle(float angle)
{
    m_snapSettings.rotationAngle = std::max(1.0f, angle);
}

void MeasurementTools::ShowGrid(bool show)
{
    m_gridVisible = show;
}

void MeasurementTools::SetGridOrigin(const Vector3& origin)
{
    m_gridOrigin = origin;
}

void MeasurementTools::SetGridSize(const Vector3& size)
{
    m_gridSize = size;
}

void MeasurementTools::SetGridColor(const Color& color)
{
    m_gridColor = color;
}

void MeasurementTools::ActivateRuler(const Vector3& startPoint)
{
    m_rulerActive = true;
    m_rulerStart = startPoint;
    m_rulerEnd = startPoint;
}

void MeasurementTools::DeactivateRuler()
{
    m_rulerActive = false;
}

void MeasurementTools::UpdateRuler(const Vector3& endPoint)
{
    if (m_rulerActive)
    {
        m_rulerEnd = endPoint;
    }
}

void MeasurementTools::ActivateProtractor(const Vector3& centerPoint)
{
    m_protractorActive = true;
    m_anglePoints.clear();
    m_anglePoints.push_back(centerPoint);
}

void MeasurementTools::AddProtractorPoint(const Vector3& point)
{
    if (m_protractorActive)
    {
        m_anglePoints.push_back(point);
    }
}

void MeasurementTools::DeactivateProtractor()
{
    m_protractorActive = false;
    m_anglePoints.clear();
}

void MeasurementTools::ActivateAreaMeasurement()
{
    m_areaMeasurementActive = true;
    m_areaPoints.clear();
}

void MeasurementTools::AddAreaPoint(const Vector3& point)
{
    if (m_areaMeasurementActive)
    {
        m_areaPoints.push_back(point);
    }
}

void MeasurementTools::CompleteAreaMeasurement()
{
    if (m_areaMeasurementActive && m_areaPoints.size() >= 3)
    {
        // Create area measurement
        int measurementIndex = StartMeasurement(MeasurementType::AREA, "Area Measurement");
        for (const auto& point : m_areaPoints)
        {
            AddMeasurementPoint(measurementIndex, point);
        }
        CompleteMeasurement(measurementIndex);
    }

    m_areaMeasurementActive = false;
    m_areaPoints.clear();
}

void MeasurementTools::DeactivateAreaMeasurement()
{
    m_areaMeasurementActive = false;
    m_areaPoints.clear();
}

Vector3 MeasurementTools::GetClosestVertex(const Vector3& point, const std::vector<MapObject>& objects) const
{
    Vector3 closest = point;
    float minDistance = m_snapSettings.snapTolerance;

    for (const auto& object : objects)
    {
        // Get object vertices (this would depend on object type)
        Vector3 objPos = object.GetPosition();

        // For cubes, check the 8 vertices
        if (object.GetType() == 0) // Cube
        {
            Vector3 halfSize = Vector3Scale(object.GetScale(), 0.5f);
            for (int i = -1; i <= 1; i += 2)
            {
                for (int j = -1; j <= 1; j += 2)
                {
                    for (int k = -1; k <= 1; k += 2)
                    {
                        Vector3 vertex = {
                            objPos.x + i * halfSize.x,
                            objPos.y + j * halfSize.y,
                            objPos.z + k * halfSize.z
                        };

                        float distance = Vector3Distance(point, vertex);
                        if (distance < minDistance)
                        {
                            minDistance = distance;
                            closest = vertex;
                        }
                    }
                }
            }
        }
    }

    return closest;
}

Vector3 MeasurementTools::GetClosestEdge(const Vector3& point, const std::vector<MapObject>& objects) const
{
    Vector3 closest = point;
    float minDistance = m_snapSettings.snapTolerance;

    for (const auto& object : objects)
    {
        if (object.GetType() == 0) // Cube
        {
            Vector3 objPos = object.GetPosition();
            Vector3 halfSize = Vector3Scale(object.GetScale(), 0.5f);

            // Check all 12 edges of the cube
            Vector3 vertices[8] = {
                {objPos.x - halfSize.x, objPos.y - halfSize.y, objPos.z - halfSize.z},
                {objPos.x + halfSize.x, objPos.y - halfSize.y, objPos.z - halfSize.z},
                {objPos.x + halfSize.x, objPos.y + halfSize.y, objPos.z - halfSize.z},
                {objPos.x - halfSize.x, objPos.y + halfSize.y, objPos.z - halfSize.z},
                {objPos.x - halfSize.x, objPos.y - halfSize.y, objPos.z + halfSize.z},
                {objPos.x + halfSize.x, objPos.y - halfSize.y, objPos.z + halfSize.z},
                {objPos.x + halfSize.x, objPos.y + halfSize.y, objPos.z + halfSize.z},
                {objPos.x - halfSize.x, objPos.y + halfSize.y, objPos.z + halfSize.z}
            };

            int edges[12][2] = {
                {0, 1}, {1, 2}, {2, 3}, {3, 0}, // Bottom face
                {4, 5}, {5, 6}, {6, 7}, {7, 4}, // Top face
                {0, 4}, {1, 5}, {2, 6}, {3, 7}  // Vertical edges
            };

            for (int i = 0; i < 12; ++i)
            {
                Vector3 edgeStart = vertices[edges[i][0]];
                Vector3 edgeEnd = vertices[edges[i][1]];
                Vector3 edgePoint = Vector3ClosestPointOnLine(point, edgeStart, edgeEnd);

                float distance = Vector3Distance(point, edgePoint);
                if (distance < minDistance)
                {
                    minDistance = distance;
                    closest = edgePoint;
                }
            }
        }
    }

    return closest;
}

Vector3 MeasurementTools::GetClosestFace(const Vector3& point, const std::vector<MapObject>& objects) const
{
    // Simplified face snapping - would need more complex implementation for full functionality
    return point;
}

Vector3 MeasurementTools::GetClosestObjectCenter(const Vector3& point, const std::vector<MapObject>& objects) const
{
    Vector3 closest = point;
    float minDistance = m_snapSettings.snapTolerance;

    for (const auto& object : objects)
    {
        Vector3 objCenter = object.GetPosition();
        float distance = Vector3Distance(point, objCenter);

        if (distance < minDistance)
        {
            minDistance = distance;
            closest = objCenter;
        }
    }

    return closest;
}

float MeasurementTools::CalculateDistance(const Vector3& point1, const Vector3& point2) const
{
    return Vector3Distance(point1, point2);
}

float MeasurementTools::CalculateAngle(const Vector3& center, const Vector3& point1, const Vector3& point2) const
{
    Vector3 vec1 = Vector3Subtract(point1, center);
    Vector3 vec2 = Vector3Subtract(point2, center);

    float dot = Vector3DotProduct(vec1, vec2);
    float len1 = Vector3Length(vec1);
    float len2 = Vector3Length(vec2);

    if (len1 == 0.0f || len2 == 0.0f)
        return 0.0f;

    float cosAngle = dot / (len1 * len2);
    cosAngle = std::max(-1.0f, std::min(1.0f, cosAngle));

    return acosf(cosAngle) * RAD2DEG;
}

float MeasurementTools::CalculateArea(const std::vector<Vector3>& points) const
{
    if (points.size() < 3)
        return 0.0f;

    // Use shoelace formula for polygon area
    float area = 0.0f;
    int n = static_cast<int>(points.size());

    for (int i = 0; i < n; ++i)
    {
        int j = (i + 1) % n;
        area += points[i].x * points[j].z;
        area -= points[j].x * points[i].z;
    }

    area = fabsf(area) / 2.0f;
    return area;
}

float MeasurementTools::CalculateVolume(const std::vector<Vector3>& points) const
{
    // Simplified volume calculation - would need proper 3D implementation
    return 0.0f;
}

Vector3 MeasurementTools::WorldToGrid(const Vector3& worldPoint) const
{
    Vector3 gridPoint = Vector3Subtract(worldPoint, m_gridOrigin);
    gridPoint.x /= m_snapSettings.gridSize;
    gridPoint.y /= m_snapSettings.gridSize;
    gridPoint.z /= m_snapSettings.gridSize;
    return gridPoint;
}

Vector3 MeasurementTools::GridToWorld(const Vector3& gridPoint) const
{
    Vector3 worldPoint = {
        gridPoint.x * m_snapSettings.gridSize + m_gridOrigin.x,
        gridPoint.y * m_snapSettings.gridSize + m_gridOrigin.y,
        gridPoint.z * m_snapSettings.gridSize + m_gridOrigin.z
    };
    return worldPoint;
}

Vector3 MeasurementTools::SnapToGrid(const Vector3& point) const
{
    Vector3 gridPoint = WorldToGrid(point);
    gridPoint.x = roundf(gridPoint.x);
    gridPoint.y = roundf(gridPoint.y);
    gridPoint.z = roundf(gridPoint.z);
    return GridToWorld(gridPoint);
}

void MeasurementTools::Render()
{
    if (m_gridVisible)
    {
        RenderGrid();
    }

    RenderMeasurements();

    if (m_rulerActive)
    {
        RenderRuler();
    }

    if (m_protractorActive)
    {
        RenderProtractor();
    }

    if (m_areaMeasurementActive)
    {
        RenderAreaMeasurement();
    }
}

void MeasurementTools::RenderGrid()
{
    DrawGridLines();
    DrawGridPoints();
}

void MeasurementTools::RenderMeasurements()
{
    for (const auto& measurement : m_measurements)
    {
        if (!measurement.visible)
            continue;

        switch (measurement.type)
        {
            case MeasurementType::DISTANCE:
                if (measurement.points.size() >= 2)
                {
                    DrawLine3D(measurement.points[0], measurement.points[1], measurement.color);
                    DrawSphere(measurement.points[0], 0.05f, measurement.color);
                    DrawSphere(measurement.points[1], 0.05f, measurement.color);
                }
                break;

            case MeasurementType::ANGLE:
                if (measurement.points.size() >= 3)
                {
                    DrawLine3D(measurement.points[0], measurement.points[1], measurement.color);
                    DrawLine3D(measurement.points[0], measurement.points[2], measurement.color);
                    DrawSphere(measurement.points[0], 0.05f, measurement.color);
                    DrawSphere(measurement.points[1], 0.05f, measurement.color);
                    DrawSphere(measurement.points[2], 0.05f, measurement.color);
                }
                break;

            case MeasurementType::AREA:
                if (measurement.points.size() >= 3)
                {
                    for (size_t i = 0; i < measurement.points.size(); ++i)
                    {
                        size_t j = (i + 1) % measurement.points.size();
                        DrawLine3D(measurement.points[i], measurement.points[j], measurement.color);
                        DrawSphere(measurement.points[i], 0.05f, measurement.color);
                    }
                }
                break;

            case MeasurementType::VOLUME:
                // Volume rendering would be more complex
                break;

            case MeasurementType::COORDINATES:
                if (!measurement.points.empty())
                {
                    DrawSphere(measurement.points[0], 0.05f, measurement.color);
                }
                break;
        }
    }
}

void MeasurementTools::RenderRuler()
{
    DrawLine3D(m_rulerStart, m_rulerEnd, YELLOW);
    DrawSphere(m_rulerStart, 0.05f, YELLOW);
    DrawSphere(m_rulerEnd, 0.05f, YELLOW);

    // Draw distance text
    Vector3 midpoint = Vector3Lerp(m_rulerStart, m_rulerEnd, 0.5f);
    float distance = Vector3Distance(m_rulerStart, m_rulerEnd);

    // Simple distance display (in a real implementation, you'd use a proper text rendering system)
    std::string distanceText = std::to_string(distance);
}

void MeasurementTools::RenderProtractor()
{
    if (m_anglePoints.size() >= 2)
    {
        DrawLine3D(m_anglePoints[0], m_anglePoints[1], GREEN);
        DrawSphere(m_anglePoints[0], 0.05f, GREEN);

        if (m_anglePoints.size() >= 3)
        {
            DrawLine3D(m_anglePoints[0], m_anglePoints[2], GREEN);
            DrawSphere(m_anglePoints[2], 0.05f, GREEN);

            // Draw arc for angle visualization
            float angle = CalculateAngle(m_anglePoints[0], m_anglePoints[1], m_anglePoints[2]);
            // Arc drawing would be implemented here
        }
    }
}

void MeasurementTools::RenderAreaMeasurement()
{
    for (size_t i = 0; i < m_areaPoints.size(); ++i)
    {
        size_t j = (i + 1) % m_areaPoints.size();
        DrawLine3D(m_areaPoints[i], m_areaPoints[j], BLUE);
        DrawSphere(m_areaPoints[i], 0.05f, BLUE);
    }
}

void MeasurementTools::RenderSnapPreview(const Vector3& point)
{
    if (!m_snappingEnabled)
        return;

    DrawSphere(point, 0.03f, GREEN);
}

std::string MeasurementTools::SerializeMeasurements() const
{
    // Serialize measurements to string format
    return "Measurements data";
}

bool MeasurementTools::DeserializeMeasurements(const std::string& data)
{
    // Deserialize measurements from string
    return true;
}

void MeasurementTools::UpdateMeasurement(int index)
{
    if (index < 0 || index >= static_cast<int>(m_measurements.size()))
        return;

    Measurement& measurement = m_measurements[index];

    switch (measurement.type)
    {
        case MeasurementType::DISTANCE:
            CalculateDistanceMeasurement(index);
            break;
        case MeasurementType::ANGLE:
            CalculateAngleMeasurement(index);
            break;
        case MeasurementType::AREA:
            CalculateAreaMeasurement(index);
            break;
        case MeasurementType::VOLUME:
            CalculateVolumeMeasurement(index);
            break;
        case MeasurementType::COORDINATES:
            // Coordinates don't need calculation
            break;
    }
}

Vector3 MeasurementTools::FindSnapPoint(const Vector3& point, const std::vector<MapObject>& objects)
{
    Vector3 snapPoint = point;

    switch (m_snapSettings.mode)
    {
        case SnapMode::GRID:
            snapPoint = SnapToGrid(point);
            break;

        case SnapMode::VERTEX:
            if (m_snapSettings.snapToVertices)
            {
                snapPoint = GetClosestVertex(point, objects);
            }
            break;

        case SnapMode::EDGE:
            if (m_snapSettings.snapToEdges)
            {
                snapPoint = GetClosestEdge(point, objects);
            }
            break;

        case SnapMode::FACE:
            if (m_snapSettings.snapToFaces)
            {
                snapPoint = GetClosestFace(point, objects);
            }
            break;

        case SnapMode::OBJECT_CENTER:
            if (m_snapSettings.snapToObjectCenters)
            {
                snapPoint = GetClosestObjectCenter(point, objects);
            }
            break;

        case SnapMode::WORLD_AXES:
            if (m_snapSettings.snapToWorldAxes)
            {
                // Snap to world axis planes
                Vector3 gridPoint = WorldToGrid(point);
                if (fabsf(gridPoint.x) < m_snapSettings.snapTolerance)
                    gridPoint.x = 0.0f;
                if (fabsf(gridPoint.y) < m_snapSettings.snapTolerance)
                    gridPoint.y = 0.0f;
                if (fabsf(gridPoint.z) < m_snapSettings.snapTolerance)
                    gridPoint.z = 0.0f;
                snapPoint = GridToWorld(gridPoint);
            }
            break;

        case SnapMode::NONE:
        default:
            snapPoint = point;
            break;
    }

    return snapPoint;
}

bool MeasurementTools::IsPointOnGrid(const Vector3& point) const
{
    Vector3 gridPoint = WorldToGrid(point);
    return fabsf(gridPoint.x - roundf(gridPoint.x)) < 0.01f &&
           fabsf(gridPoint.y - roundf(gridPoint.y)) < 0.01f &&
           fabsf(gridPoint.z - roundf(gridPoint.z)) < 0.01f;
}

Vector3 MeasurementTools::GetGridSnapPoint(const Vector3& point) const
{
    return SnapToGrid(point);
}

void MeasurementTools::CalculateDistanceMeasurement(int index)
{
    if (index < 0 || index >= static_cast<int>(m_measurements.size()) ||
        m_measurements[index].points.size() < 2)
        return;

    const std::vector<Vector3>& points = m_measurements[index].points;
    m_measurements[index].value = CalculateDistance(points[0], points[1]);
}

void MeasurementTools::CalculateAngleMeasurement(int index)
{
    if (index < 0 || index >= static_cast<int>(m_measurements.size()) ||
        m_measurements[index].points.size() < 3)
        return;

    const std::vector<Vector3>& points = m_measurements[index].points;
    m_measurements[index].value = CalculateAngle(points[0], points[1], points[2]);
}

void MeasurementTools::CalculateAreaMeasurement(int index)
{
    if (index < 0 || index >= static_cast<int>(m_measurements.size()) ||
        m_measurements[index].points.size() < 3)
        return;

    const std::vector<Vector3>& points = m_measurements[index].points;
    m_measurements[index].value = CalculateArea(points);
}

void MeasurementTools::CalculateVolumeMeasurement(int index)
{
    if (index < 0 || index >= static_cast<int>(m_measurements.size()) ||
        m_measurements[index].points.size() < 4)
        return;

    const std::vector<Vector3>& points = m_measurements[index].points;
    m_measurements[index].value = CalculateVolume(points);
}

void MeasurementTools::DrawGridLines()
{
    // Draw grid lines in XZ plane at Y = 0
    int linesX = static_cast<int>(m_gridSize.x);
    int linesZ = static_cast<int>(m_gridSize.z);

    for (int i = -linesX; i <= linesX; ++i)
    {
        Vector3 start = {m_gridOrigin.x + i * m_snapSettings.gridSize, m_gridOrigin.y, m_gridOrigin.z - m_gridSize.z * m_snapSettings.gridSize};
        Vector3 end = {m_gridOrigin.x + i * m_snapSettings.gridSize, m_gridOrigin.y, m_gridOrigin.z + m_gridSize.z * m_snapSettings.gridSize};

        Color lineColor = GetGridLineColor(i);
        DrawLine3D(start, end, lineColor);
    }

    for (int i = -linesZ; i <= linesZ; ++i)
    {
        Vector3 start = {m_gridOrigin.x - m_gridSize.x * m_snapSettings.gridSize, m_gridOrigin.y, m_gridOrigin.z + i * m_snapSettings.gridSize};
        Vector3 end = {m_gridOrigin.x + m_gridSize.x * m_snapSettings.gridSize, m_gridOrigin.y, m_gridOrigin.z + i * m_snapSettings.gridSize};

        Color lineColor = GetGridLineColor(i);
        DrawLine3D(start, end, lineColor);
    }
}

void MeasurementTools::DrawGridPoints()
{
    // Draw grid intersection points
    int pointsX = static_cast<int>(m_gridSize.x * 2);
    int pointsZ = static_cast<int>(m_gridSize.z * 2);

    for (int x = -pointsX; x <= pointsX; ++x)
    {
        for (int z = -pointsZ; z <= pointsZ; ++z)
        {
            Vector3 point = {
                m_gridOrigin.x + x * m_snapSettings.gridSize * 0.5f,
                m_gridOrigin.y,
                m_gridOrigin.z + z * m_snapSettings.gridSize * 0.5f
            };

            // Only draw points at integer grid positions
            if (fabsf(fmodf(point.x, m_snapSettings.gridSize)) < 0.01f &&
                fabsf(fmodf(point.z, m_snapSettings.gridSize)) < 0.01f)
            {
                DrawSphere(point, 0.02f, Fade(m_gridColor, 0.5f));
            }
        }
    }
}

Color MeasurementTools::GetGridLineColor(int lineIndex) const
{
    if (lineIndex == 0)
    {
        return Fade(RED, 0.8f); // X axis
    }
    else if (lineIndex == 0)
    {
        return Fade(GREEN, 0.8f); // Z axis (would need separate check)
    }
    else
    {
        return Fade(m_gridColor, 0.3f);
    }
}