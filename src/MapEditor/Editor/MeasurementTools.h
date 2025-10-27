#ifndef MEASUREMENTTOOLS_H
#define MEASUREMENTTOOLS_H

#include <raylib.h>
#include <vector>
#include <string>
#include <memory>

#include "MapObject.h"

// Measurement types
enum class MeasurementType
{
    DISTANCE = 0,
    ANGLE = 1,
    AREA = 2,
    VOLUME = 3,
    COORDINATES = 4
};

// Snapping modes
enum class SnapMode
{
    NONE = 0,
    GRID = 1,
    VERTEX = 2,
    EDGE = 3,
    FACE = 4,
    OBJECT_CENTER = 5,
    WORLD_AXES = 6
};

// Measurement data
struct Measurement
{
    MeasurementType type;
    std::string name;
    std::vector<Vector3> points;
    float value;
    std::string unit;
    Color color;
    bool visible;

    Measurement() : type(MeasurementType::DISTANCE), value(0.0f), color(YELLOW), visible(true) {}
};

// Snap settings
struct SnapSettings
{
    SnapMode mode;
    float gridSize;
    bool snapToVertices;
    bool snapToEdges;
    bool snapToFaces;
    bool snapToObjectCenters;
    bool snapToWorldAxes;
    float snapTolerance;
    bool snapRotation;
    float rotationAngle;

    SnapSettings() :
        mode(SnapMode::GRID),
        gridSize(1.0f),
        snapToVertices(true),
        snapToEdges(true),
        snapToFaces(false),
        snapToObjectCenters(true),
        snapToWorldAxes(true),
        snapTolerance(0.1f),
        snapRotation(false),
        rotationAngle(15.0f)
    {}
};

// Measurement and snapping tools
class MeasurementTools
{
private:
    std::vector<Measurement> m_measurements;
    int m_activeMeasurement;
    bool m_measurementMode;

    // Snapping system
    SnapSettings m_snapSettings;
    Vector3 m_snapOffset;
    bool m_snappingEnabled;

    // Grid system
    bool m_gridVisible;
    Vector3 m_gridOrigin;
    Vector3 m_gridSize;
    Color m_gridColor;

    // Ruler tool
    bool m_rulerActive;
    Vector3 m_rulerStart;
    Vector3 m_rulerEnd;

    // Protractor tool
    bool m_protractorActive;
    std::vector<Vector3> m_anglePoints;

    // Area measurement
    bool m_areaMeasurementActive;
    std::vector<Vector3> m_areaPoints;

public:
    MeasurementTools();
    ~MeasurementTools() = default;

    // Initialization
    void Initialize();
    void Reset();

    // Measurement functions
    int StartMeasurement(MeasurementType type, const std::string& name = "");
    bool AddMeasurementPoint(int measurementIndex, const Vector3& point);
    bool CompleteMeasurement(int measurementIndex);
    bool DeleteMeasurement(int index);
    void ClearAllMeasurements();

    // Measurement queries
    float GetMeasurementValue(int index) const;
    std::string GetMeasurementInfo(int index) const;
    Vector3 GetMeasurementPoint(int measurementIndex, int pointIndex) const;

    // Active measurement
    void SetActiveMeasurement(int index);
    int GetActiveMeasurement() const { return m_activeMeasurement; }
    bool IsMeasurementMode() const { return m_measurementMode; }

    // Snapping functions
    Vector3 SnapPoint(const Vector3& point, const std::vector<MapObject>& objects);
    Vector3 SnapRotation(const Vector3& rotation);
    bool IsSnappingEnabled() const { return m_snappingEnabled; }
    void EnableSnapping(bool enable);

    // Snap settings
    void SetSnapMode(SnapMode mode);
    void SetGridSize(float size);
    void SetSnapTolerance(float tolerance);
    void SetSnapToVertices(bool enable);
    void SetSnapToEdges(bool enable);
    void SetSnapToFaces(bool enable);
    void SetSnapToObjectCenters(bool enable);
    void SetSnapRotation(bool enable);
    void SetRotationAngle(float angle);

    // Grid functions
    void ShowGrid(bool show);
    void SetGridOrigin(const Vector3& origin);
    void SetGridSize(const Vector3& size);
    void SetGridColor(const Color& color);
    bool IsGridVisible() const { return m_gridVisible; }

    // Tool activation
    void ActivateRuler(const Vector3& startPoint);
    void DeactivateRuler();
    void UpdateRuler(const Vector3& endPoint);
    bool IsRulerActive() const { return m_rulerActive; }

    void ActivateProtractor(const Vector3& centerPoint);
    void AddProtractorPoint(const Vector3& point);
    void DeactivateProtractor();
    bool IsProtractorActive() const { return m_protractorActive; }

    void ActivateAreaMeasurement();
    void AddAreaPoint(const Vector3& point);
    void CompleteAreaMeasurement();
    void DeactivateAreaMeasurement();
    bool IsAreaMeasurementActive() const { return m_areaMeasurementActive; }

    // Utility functions
    Vector3 GetClosestVertex(const Vector3& point, const std::vector<MapObject>& objects) const;
    Vector3 GetClosestEdge(const Vector3& point, const std::vector<MapObject>& objects) const;
    Vector3 GetClosestFace(const Vector3& point, const std::vector<MapObject>& objects) const;
    Vector3 GetClosestObjectCenter(const Vector3& point, const std::vector<MapObject>& objects) const;

    // Distance calculations
    float CalculateDistance(const Vector3& point1, const Vector3& point2) const;
    float CalculateAngle(const Vector3& center, const Vector3& point1, const Vector3& point2) const;
    float CalculateArea(const std::vector<Vector3>& points) const;
    float CalculateVolume(const std::vector<Vector3>& points) const;

    // Coordinate system conversions
    Vector3 WorldToGrid(const Vector3& worldPoint) const;
    Vector3 GridToWorld(const Vector3& gridPoint) const;
    Vector3 SnapToGrid(const Vector3& point) const;

    // Rendering
    void Render();
    void RenderGrid();
    void RenderMeasurements();
    void RenderRuler();
    void RenderProtractor();
    void RenderAreaMeasurement();
    void RenderSnapPreview(const Vector3& point);

    // Serialization
    std::string SerializeMeasurements() const;
    bool DeserializeMeasurements(const std::string& data);

private:
    // Helper functions
    void UpdateMeasurement(int index);
    Vector3 FindSnapPoint(const Vector3& point, const std::vector<MapObject>& objects);
    bool IsPointOnGrid(const Vector3& point) const;
    Vector3 GetGridSnapPoint(const Vector3& point) const;

    // Measurement calculations
    void CalculateDistanceMeasurement(int index);
    void CalculateAngleMeasurement(int index);
    void CalculateAreaMeasurement(int index);
    void CalculateVolumeMeasurement(int index);

    // Grid utilities
    void DrawGridLines();
    void DrawGridPoints();
    Color GetGridLineColor(int lineIndex) const;
};

#endif // MEASUREMENTTOOLS_H