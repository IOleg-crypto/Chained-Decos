#ifndef ADVANCED_EDITOR_TOOLS_H
#define ADVANCED_EDITOR_TOOLS_H

#include <vector>
#include <string>
#include <unordered_map>
#include <raylib.h>
#include "MapObject.h"

// Template system for object configurations
struct ObjectTemplate {
    std::string name;
    std::string category;
    MapObject baseObject;
    std::string description;
    std::string thumbnailPath;
    std::unordered_map<std::string, std::string> metadata;

    ObjectTemplate(const std::string& templateName, const MapObject& obj);
};

struct BatchOperation {
    enum class OperationType {
        TRANSLATE,
        ROTATE,
        SCALE,
        SET_PROPERTY,
        DELETE,
        DUPLICATE,
        APPLY_MATERIAL
    };

    OperationType type;
    std::vector<int> targetObjectIndices;
    Vector3 translation;
    Vector3 rotation;
    Vector3 scale;
    std::string propertyName;
    std::string propertyValue;
    std::string materialName;

    BatchOperation(OperationType opType, const std::vector<int>& targets);
};

struct AlignmentGuide {
    enum class GuideType {
        VERTICAL,
        HORIZONTAL,
        DEPTH,
        CUSTOM_PLANE
    };

    GuideType type;
    Vector3 position;
    Vector3 normal;
    Color color;
    bool enabled;
    float snapDistance;

    AlignmentGuide(GuideType guideType, const Vector3& pos, const Vector3& norm = {0,1,0});
};

class AdvancedEditorTools {
public:
    AdvancedEditorTools();
    ~AdvancedEditorTools() = default;

    // Template system
    void SaveObjectAsTemplate(const MapObject& object, const std::string& templateName,
                             const std::string& category = "Custom");
    bool LoadTemplate(const std::string& templateName, MapObject& outObject);
    void DeleteTemplate(const std::string& templateName);
    std::vector<std::string> GetTemplateNames(const std::string& category = "") const;
    std::vector<std::string> GetTemplateCategories() const;
    ObjectTemplate* GetTemplate(const std::string& templateName);

    // Batch operations
    void ExecuteBatchOperation(const BatchOperation& operation);
    void UndoLastBatchOperation();
    void ClearBatchHistory();
    bool CanUndoBatchOperation() const;

    // Alignment system
    void AddAlignmentGuide(const AlignmentGuide& guide);
    void RemoveAlignmentGuide(size_t index);
    void ClearAlignmentGuides();
    Vector3 SnapToAlignmentGuides(const Vector3& position, float snapDistance = 0.5f);
    void RenderAlignmentGuides() const;
    void EnableAlignmentGuides(bool enable);
    bool AreAlignmentGuidesEnabled() const;

    // Advanced snapping
    Vector3 SmartSnap(const Vector3& position, const std::vector<MapObject>& objects,
                     float snapDistance = 0.5f);
    void SetSnapDistance(float distance);
    float GetSnapDistance() const;

    // Measurement and dimension tools
    void MeasureDistance(const Vector3& start, const Vector3& end);
    void MeasureAngle(const Vector3& center, const Vector3& point1, const Vector3& point2);
    void MeasureArea(const std::vector<Vector3>& points);
    void ClearMeasurements();
    std::string GetLastMeasurement() const;

    // Object arrangement tools
    void AlignObjects(const std::vector<int>& objectIndices, const std::string& alignment);
    void DistributeObjects(const std::vector<int>& objectIndices, const std::string& axis);
    void ArrangeInGrid(const std::vector<int>& objectIndices, int rows, int cols, float spacing);
    void ArrangeInCircle(const std::vector<int>& objectIndices, const Vector3& center, float radius);

    // Advanced selection tools
    void SelectByType(MapObjectType type);
    void SelectByMaterial(const std::string& materialName);
    void SelectBySize(const Vector3& minSize, const Vector3& maxSize);
    void SelectByDistance(const Vector3& center, float radius);
    void InvertSelection();
    void ExpandSelection();

    // Material and texture tools
    void ApplyMaterialToSelection(const std::string& materialName);
    void ReplaceTexture(const std::string& oldTexture, const std::string& newTexture);
    void BatchRenameObjects(const std::string& prefix, int startNumber = 1);

    // Statistics and analysis
    struct EditorStatistics {
        int totalObjects;
        int objectsByType[static_cast<int>(MapObjectType::COUNT)];
        float totalArea;
        float totalVolume;
        Vector3 boundsMin;
        Vector3 boundsMax;
        std::unordered_map<std::string, int> materialUsage;
    };

    EditorStatistics CalculateStatistics(const std::vector<MapObject>& objects) const;
    void ExportStatistics(const std::string& filename) const;

    // Performance monitoring
    void StartPerformanceMonitoring();
    void StopPerformanceMonitoring();
    struct PerformanceMetrics {
        float averageFrameTime;
        int totalOperations;
        float memoryUsage;
        int undoStackSize;
    };
    PerformanceMetrics GetPerformanceMetrics() const;

private:
    // Template storage
    std::vector<ObjectTemplate> m_templates;
    std::unordered_map<std::string, size_t> m_templateIndex;

    // Batch operation history
    std::vector<BatchOperation> m_batchHistory;
    std::vector<std::vector<MapObject>> m_objectSnapshots; // For undo

    // Alignment guides
    std::vector<AlignmentGuide> m_alignmentGuides;
    bool m_alignmentGuidesEnabled;

    // Advanced snapping
    float m_snapDistance;

    // Measurements
    std::vector<std::string> m_measurements;

    // Performance monitoring
    bool m_monitoringPerformance;
    std::vector<float> m_frameTimes;
    size_t m_operationCount;

    // Helper methods
    void SaveTemplateToFile(const ObjectTemplate& temp, const std::string& filename);
    bool LoadTemplateFromFile(const std::string& filename, ObjectTemplate& temp);
    void StoreObjectSnapshot(const std::vector<int>& indices, const std::vector<MapObject>& objects);
    void RestoreObjectSnapshot(size_t snapshotIndex, std::vector<MapObject>& objects);
    Vector3 CalculateAlignmentSnap(const Vector3& position, const AlignmentGuide& guide);
    bool IsPointOnGuide(const Vector3& point, const AlignmentGuide& guide, float tolerance);
};

#endif // ADVANCED_EDITOR_TOOLS_H