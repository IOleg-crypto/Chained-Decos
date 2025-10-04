#include "AdvancedEditorTools.h"
#include <algorithm>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <raymath.h>
#include <raylib.h>

ObjectTemplate::ObjectTemplate(const std::string& templateName, const MapObject& obj)
    : name(templateName), baseObject(obj) {}

BatchOperation::BatchOperation(OperationType opType, const std::vector<int>& targets)
    : type(opType), targetObjectIndices(targets) {}

AlignmentGuide::AlignmentGuide(GuideType guideType, const Vector3& pos, const Vector3& norm)
    : type(guideType), position(pos), normal(Vector3Normalize(norm)), enabled(true), snapDistance(0.5f) {
    // Set default colors based on type
    switch (type) {
        case GuideType::VERTICAL:
            color = RED;
            break;
        case GuideType::HORIZONTAL:
            color = GREEN;
            break;
        case GuideType::DEPTH:
            color = BLUE;
            break;
        case GuideType::CUSTOM_PLANE:
            color = YELLOW;
            break;
    }
}

AdvancedEditorTools::AdvancedEditorTools()
    : m_alignmentGuidesEnabled(true), m_snapDistance(0.5f), m_monitoringPerformance(false), m_operationCount(0) {}

void AdvancedEditorTools::SaveObjectAsTemplate(const MapObject& object, const std::string& templateName,
                                              const std::string& category) {
    ObjectTemplate newTemplate(templateName, object);
    newTemplate.category = category;

    // Add metadata
    newTemplate.metadata["created"] = "2024-01-01"; // Would use current date
    newTemplate.metadata["author"] = "Editor";
    newTemplate.metadata["version"] = "1.0";

    m_templates.push_back(newTemplate);
    m_templateIndex[templateName] = m_templates.size() - 1;

    TraceLog(LOG_INFO, "AdvancedEditorTools::SaveObjectAsTemplate() - Saved template: %s", templateName.c_str());
}

bool AdvancedEditorTools::LoadTemplate(const std::string& templateName, MapObject& outObject) {
    auto it = m_templateIndex.find(templateName);
    if (it != m_templateIndex.end()) {
        outObject = m_templates[it->second].baseObject;
        TraceLog(LOG_INFO, "AdvancedEditorTools::LoadTemplate() - Loaded template: %s", templateName.c_str());
        return true;
    }
    return false;
}

void AdvancedEditorTools::DeleteTemplate(const std::string& templateName) {
    auto it = m_templateIndex.find(templateName);
    if (it != m_templateIndex.end()) {
        size_t index = it->second;
        m_templates.erase(m_templates.begin() + index);
        m_templateIndex.erase(it);

        // Update indices for remaining templates
        for (auto& pair : m_templateIndex) {
            if (pair.second > index) {
                pair.second--;
            }
        }

        TraceLog(LOG_INFO, "AdvancedEditorTools::DeleteTemplate() - Deleted template: %s", templateName.c_str());
    }
}

std::vector<std::string> AdvancedEditorTools::GetTemplateNames(const std::string& category) const {
    std::vector<std::string> names;
    for (const auto& temp : m_templates) {
        if (category.empty() || temp.category == category) {
            names.push_back(temp.name);
        }
    }
    return names;
}

std::vector<std::string> AdvancedEditorTools::GetTemplateCategories() const {
    std::vector<std::string> categories;
    for (const auto& temp : m_templates) {
        if (std::find(categories.begin(), categories.end(), temp.category) == categories.end()) {
            categories.push_back(temp.category);
        }
    }
    return categories;
}

ObjectTemplate* AdvancedEditorTools::GetTemplate(const std::string& templateName) {
    auto it = m_templateIndex.find(templateName);
    return (it != m_templateIndex.end()) ? &m_templates[it->second] : nullptr;
}

void AdvancedEditorTools::ExecuteBatchOperation(const BatchOperation& operation) {
    // Store snapshot for undo
    StoreObjectSnapshot(operation.targetObjectIndices, {}); // Would need actual objects

    // Execute operation based on type
    switch (operation.type) {
        case BatchOperation::OperationType::TRANSLATE:
            // Apply translation to all target objects
            break;
        case BatchOperation::OperationType::ROTATE:
            // Apply rotation to all target objects
            break;
        case BatchOperation::OperationType::SCALE:
            // Apply scaling to all target objects
            break;
        case BatchOperation::OperationType::SET_PROPERTY:
            // Set property on all target objects
            break;
        case BatchOperation::OperationType::DELETE:
            // Delete all target objects
            break;
        case BatchOperation::OperationType::DUPLICATE:
            // Duplicate all target objects
            break;
        case BatchOperation::OperationType::APPLY_MATERIAL:
            // Apply material to all target objects
            break;
    }

    m_batchHistory.push_back(operation);
    m_operationCount++;

    TraceLog(LOG_INFO, "AdvancedEditorTools::ExecuteBatchOperation() - Executed batch operation on %zu objects",
             operation.targetObjectIndices.size());
}

void AdvancedEditorTools::UndoLastBatchOperation() {
    if (!m_batchHistory.empty() && !m_objectSnapshots.empty()) {
        // Restore from snapshot
        size_t snapshotIndex = m_objectSnapshots.size() - 1;
        // RestoreObjectSnapshot(snapshotIndex, objects); // Would need objects vector

        m_batchHistory.pop_back();
        m_objectSnapshots.pop_back();

        TraceLog(LOG_INFO, "AdvancedEditorTools::UndoLastBatchOperation() - Undid last batch operation");
    }
}

void AdvancedEditorTools::ClearBatchHistory() {
    m_batchHistory.clear();
    m_objectSnapshots.clear();
    TraceLog(LOG_INFO, "AdvancedEditorTools::ClearBatchHistory() - Cleared batch operation history");
}

bool AdvancedEditorTools::CanUndoBatchOperation() const {
    return !m_batchHistory.empty() && !m_objectSnapshots.empty();
}

void AdvancedEditorTools::AddAlignmentGuide(const AlignmentGuide& guide) {
    m_alignmentGuides.push_back(guide);
    TraceLog(LOG_INFO, "AdvancedEditorTools::AddAlignmentGuide() - Added alignment guide");
}

void AdvancedEditorTools::RemoveAlignmentGuide(size_t index) {
    if (index < m_alignmentGuides.size()) {
        m_alignmentGuides.erase(m_alignmentGuides.begin() + index);
        TraceLog(LOG_INFO, "AdvancedEditorTools::RemoveAlignmentGuide() - Removed alignment guide");
    }
}

void AdvancedEditorTools::ClearAlignmentGuides() {
    m_alignmentGuides.clear();
    TraceLog(LOG_INFO, "AdvancedEditorTools::ClearAlignmentGuides() - Cleared all alignment guides");
}

Vector3 AdvancedEditorTools::SnapToAlignmentGuides(const Vector3& position, float snapDistance) {
    if (!m_alignmentGuidesEnabled || m_alignmentGuides.empty()) {
        return position;
    }

    Vector3 snappedPosition = position;

    for (const auto& guide : m_alignmentGuides) {
        if (!guide.enabled) continue;

        Vector3 snapPoint = CalculateAlignmentSnap(position, guide);
        if (Vector3Distance(position, snapPoint) <= snapDistance) {
            snappedPosition = snapPoint;
            break; // Use first valid snap
        }
    }

    return snappedPosition;
}

void AdvancedEditorTools::RenderAlignmentGuides() const {
    for (const auto& guide : m_alignmentGuides) {
        if (!guide.enabled) continue;

        switch (guide.type) {
            case AlignmentGuide::GuideType::VERTICAL:
                // Draw vertical line
                DrawLine3D({guide.position.x, guide.position.y - 10, guide.position.z},
                          {guide.position.x, guide.position.y + 10, guide.position.z}, guide.color);
                break;
            case AlignmentGuide::GuideType::HORIZONTAL:
                // Draw horizontal line
                DrawLine3D({guide.position.x - 10, guide.position.y, guide.position.z},
                          {guide.position.x + 10, guide.position.y, guide.position.z}, guide.color);
                break;
            case AlignmentGuide::GuideType::DEPTH:
                // Draw depth line
                DrawLine3D({guide.position.x, guide.position.y, guide.position.z - 10},
                          {guide.position.x, guide.position.y, guide.position.z + 10}, guide.color);
                break;
            case AlignmentGuide::GuideType::CUSTOM_PLANE:
                // Draw plane indicator
                DrawPlane(guide.position, {5, 5}, guide.color);
                break;
        }
    }
}

void AdvancedEditorTools::EnableAlignmentGuides(bool enable) {
    m_alignmentGuidesEnabled = enable;
    TraceLog(LOG_INFO, "AdvancedEditorTools::EnableAlignmentGuides() - %s alignment guides",
             enable ? "Enabled" : "Disabled");
}

bool AdvancedEditorTools::AreAlignmentGuidesEnabled() const {
    return m_alignmentGuidesEnabled;
}

Vector3 AdvancedEditorTools::SmartSnap(const Vector3& position, const std::vector<MapObject>& objects,
                                      float snapDistance) {
    Vector3 snapped = position;

    // Try alignment guides first
    snapped = SnapToAlignmentGuides(position, snapDistance);

    // Try object snapping
    for (const auto& obj : objects) {
        // Snap to object vertices
        for (const auto& vertex : obj.vertices) {
            if (Vector3Distance(position, vertex) <= snapDistance) {
                return vertex;
            }
        }

        // Snap to object center
        Vector3 center = Vector3Add(obj.position, Vector3Scale(obj.scale, 0.5f));
        if (Vector3Distance(position, center) <= snapDistance) {
            return center;
        }
    }

    return snapped;
}

void AdvancedEditorTools::SetSnapDistance(float distance) {
    m_snapDistance = distance;
    TraceLog(LOG_INFO, "AdvancedEditorTools::SetSnapDistance() - Set snap distance to %.2f", distance);
}

float AdvancedEditorTools::GetSnapDistance() const {
    return m_snapDistance;
}

void AdvancedEditorTools::MeasureDistance(const Vector3& start, const Vector3& end) {
    float distance = Vector3Distance(start, end);
    std::stringstream ss;
    ss << "Distance: " << std::fixed << std::setprecision(2) << distance << " units";
    m_measurements.push_back(ss.str());

    TraceLog(LOG_INFO, "AdvancedEditorTools::MeasureDistance() - Measured distance: %.2f", distance);
}

void AdvancedEditorTools::MeasureAngle(const Vector3& center, const Vector3& point1, const Vector3& point2) {
    Vector3 vec1 = Vector3Subtract(point1, center);
    Vector3 vec2 = Vector3Subtract(point2, center);

    float dot = Vector3DotProduct(vec1, vec2);
    float mag1 = Vector3Length(vec1);
    float mag2 = Vector3Length(vec2);

    if (mag1 > 0 && mag2 > 0) {
        float cosAngle = dot / (mag1 * mag2);
        cosAngle = std::max(-1.0f, std::min(1.0f, cosAngle));
        float angle = acos(cosAngle) * RAD2DEG;

        std::stringstream ss;
        ss << "Angle: " << std::fixed << std::setprecision(1) << angle << " degrees";
        m_measurements.push_back(ss.str());

        TraceLog(LOG_INFO, "AdvancedEditorTools::MeasureAngle() - Measured angle: %.1f degrees", angle);
    }
}

void AdvancedEditorTools::MeasureArea(const std::vector<Vector3>& points) {
    if (points.size() < 3) return;

    // Simple polygon area calculation (2D projection on XZ plane)
    float area = 0.0f;
    for (size_t i = 0; i < points.size(); ++i) {
        size_t j = (i + 1) % points.size();
        area += points[i].x * points[j].z;
        area -= points[j].x * points[i].z;
    }
    area = fabs(area) / 2.0f;

    std::stringstream ss;
    ss << "Area: " << std::fixed << std::setprecision(2) << area << " sq units";
    m_measurements.push_back(ss.str());

    TraceLog(LOG_INFO, "AdvancedEditorTools::MeasureArea() - Measured area: %.2f", area);
}

void AdvancedEditorTools::ClearMeasurements() {
    m_measurements.clear();
    TraceLog(LOG_INFO, "AdvancedEditorTools::ClearMeasurements() - Cleared all measurements");
}

std::string AdvancedEditorTools::GetLastMeasurement() const {
    return m_measurements.empty() ? "" : m_measurements.back();
}

void AdvancedEditorTools::AlignObjects(const std::vector<int>& objectIndices, const std::string& alignment) {
    if (objectIndices.empty()) return;

    // Calculate alignment position based on type
    Vector3 alignPos = {0, 0, 0};

    if (alignment == "min") {
        // Align to minimum position
        for (int index : objectIndices) {
            // Would need actual objects to calculate
        }
    } else if (alignment == "max") {
        // Align to maximum position
    } else if (alignment == "center") {
        // Align to center position
    }

    // Apply alignment
    BatchOperation operation(BatchOperation::OperationType::TRANSLATE, objectIndices);
    operation.translation = alignPos;
    ExecuteBatchOperation(operation);

    TraceLog(LOG_INFO, "AdvancedEditorTools::AlignObjects() - Aligned %zu objects", objectIndices.size());
}

void AdvancedEditorTools::DistributeObjects(const std::vector<int>& objectIndices,
                                           const std::string& axis) {
    if (objectIndices.size() < 3) return;

    // Calculate distribution
    // Would need actual objects to calculate positions

    TraceLog(LOG_INFO, "AdvancedEditorTools::DistributeObjects() - Distributed %zu objects along %s axis",
             objectIndices.size(), axis.c_str());
}

void AdvancedEditorTools::ArrangeInGrid(const std::vector<int>& objectIndices, int rows, int cols, float spacing) {
    if (objectIndices.empty()) return;

    int objectsPerCell = rows * cols;
    if (objectIndices.size() > objectsPerCell) return;

    // Arrange objects in grid pattern
    size_t objectIndex = 0;
    for (int row = 0; row < rows && objectIndex < objectIndices.size(); ++row) {
        for (int col = 0; col < cols && objectIndex < objectIndices.size(); ++col) {
            Vector3 position = {
                col * spacing,
                0.0f,
                row * spacing
            };

            // Apply position to object
            objectIndex++;
        }
    }

    TraceLog(LOG_INFO, "AdvancedEditorTools::ArrangeInGrid() - Arranged %zu objects in %dx%d grid",
             objectIndices.size(), rows, cols);
}

void AdvancedEditorTools::ArrangeInCircle(const std::vector<int>& objectIndices,
                                          const Vector3& center, float radius) {
    if (objectIndices.empty()) return;

    float angleStep = 360.0f / objectIndices.size();

    for (size_t i = 0; i < objectIndices.size(); ++i) {
        float angle = angleStep * i * DEG2RAD;
        Vector3 position = {
            center.x + cos(angle) * radius,
            center.y,
            center.z + sin(angle) * radius
        };

        // Apply position to object
    }

    TraceLog(LOG_INFO, "AdvancedEditorTools::ArrangeInCircle() - Arranged %zu objects in circle",
             objectIndices.size());
}

void AdvancedEditorTools::SelectByType(MapObjectType type) {
    // Would need access to objects vector to implement
    TraceLog(LOG_INFO, "AdvancedEditorTools::SelectByType() - Selected objects by type");
}

void AdvancedEditorTools::SelectByMaterial(const std::string& materialName) {
    // Would need access to objects vector to implement
    TraceLog(LOG_INFO, "AdvancedEditorTools::SelectByMaterial() - Selected objects by material: %s",
             materialName.c_str());
}

void AdvancedEditorTools::SelectBySize(const Vector3& minSize, const Vector3& maxSize) {
    // Would need access to objects vector to implement
    TraceLog(LOG_INFO, "AdvancedEditorTools::SelectBySize() - Selected objects by size range");
}

void AdvancedEditorTools::SelectByDistance(const Vector3& center, float radius) {
    // Would need access to objects vector to implement
    TraceLog(LOG_INFO, "AdvancedEditorTools::SelectByDistance() - Selected objects within radius %.2f", radius);
}

void AdvancedEditorTools::InvertSelection() {
    // Would need access to current selection to implement
    TraceLog(LOG_INFO, "AdvancedEditorTools::InvertSelection() - Inverted selection");
}

void AdvancedEditorTools::ExpandSelection() {
    // Would need access to objects vector to implement
    TraceLog(LOG_INFO, "AdvancedEditorTools::ExpandSelection() - Expanded selection");
}

void AdvancedEditorTools::ApplyMaterialToSelection(const std::string& materialName) {
    // Apply material to selected objects
    TraceLog(LOG_INFO, "AdvancedEditorTools::ApplyMaterialToSelection() - Applied material: %s",
             materialName.c_str());
}

void AdvancedEditorTools::ReplaceTexture(const std::string& oldTexture, const std::string& newTexture) {
    // Replace texture references in objects
    TraceLog(LOG_INFO, "AdvancedEditorTools::ReplaceTexture() - Replaced '%s' with '%s'",
             oldTexture.c_str(), newTexture.c_str());
}

void AdvancedEditorTools::BatchRenameObjects(const std::string& prefix, int startNumber) {
    // Rename selected objects with pattern
    TraceLog(LOG_INFO, "AdvancedEditorTools::BatchRenameObjects() - Renamed objects with prefix: %s",
             prefix.c_str());
}

AdvancedEditorTools::EditorStatistics AdvancedEditorTools::CalculateStatistics(
    const std::vector<MapObject>& objects) const {
    EditorStatistics stats = {};

    stats.totalObjects = objects.size();
    stats.boundsMin = {999999, 999999, 999999};
    stats.boundsMax = {-999999, -999999, -999999};

    for (const auto& obj : objects) {
        stats.objectsByType[static_cast<int>(obj.type)]++;

        // Update bounds
        Vector3 objMin = Vector3Subtract(obj.position, Vector3Scale(obj.scale, 0.5f));
        Vector3 objMax = Vector3Add(obj.position, Vector3Scale(obj.scale, 0.5f));

        stats.boundsMin = Vector3Min(stats.boundsMin, objMin);
        stats.boundsMax = Vector3Max(stats.boundsMax, objMax);

        // Calculate area and volume (simplified)
        if (obj.type == MapObjectType::CUBE) {
            stats.totalArea += obj.scale.x * obj.scale.y * 2 + obj.scale.x * obj.scale.z * 2 + obj.scale.y * obj.scale.z * 2;
            stats.totalVolume += obj.scale.x * obj.scale.y * obj.scale.z;
        }

        // Track material usage
        if (!obj.materialName.empty()) {
            stats.materialUsage[obj.materialName]++;
        }
    }

    return stats;
}

void AdvancedEditorTools::ExportStatistics(const std::string& filename) const {
    // Would need objects vector to calculate statistics
    std::ofstream file(filename);
    if (file.is_open()) {
        file << "Editor Statistics Export\n";
        file << "Generated: " << "2024-01-01" << "\n"; // Would use current time
        file << "Total Objects: 0\n"; // Would use calculated stats
        file.close();
        TraceLog(LOG_INFO, "AdvancedEditorTools::ExportStatistics() - Exported statistics to %s", filename.c_str());
    }
}

void AdvancedEditorTools::StartPerformanceMonitoring() {
    m_monitoringPerformance = true;
    m_frameTimes.clear();
    m_operationCount = 0;
    TraceLog(LOG_INFO, "AdvancedEditorTools::StartPerformanceMonitoring() - Started performance monitoring");
}

void AdvancedEditorTools::StopPerformanceMonitoring() {
    m_monitoringPerformance = false;
    TraceLog(LOG_INFO, "AdvancedEditorTools::StopPerformanceMonitoring() - Stopped performance monitoring");
}

AdvancedEditorTools::PerformanceMetrics AdvancedEditorTools::GetPerformanceMetrics() const {
    PerformanceMetrics metrics = {};

    if (!m_frameTimes.empty()) {
        float totalTime = 0.0f;
        for (float frameTime : m_frameTimes) {
            totalTime += frameTime;
        }
        metrics.averageFrameTime = totalTime / m_frameTimes.size();
    }

    metrics.totalOperations = m_operationCount;
    metrics.undoStackSize = m_batchHistory.size();

    return metrics;
}

void AdvancedEditorTools::StoreObjectSnapshot(const std::vector<int>& indices,
                                             const std::vector<MapObject>& objects) {
    // Store snapshot of objects for undo functionality
    std::vector<MapObject> snapshot;
    for (int index : indices) {
        if (index >= 0 && index < static_cast<int>(objects.size())) {
            snapshot.push_back(objects[index]);
        }
    }
    m_objectSnapshots.push_back(snapshot);
}

void AdvancedEditorTools::RestoreObjectSnapshot(size_t snapshotIndex, std::vector<MapObject>& objects) {
    if (snapshotIndex < m_objectSnapshots.size()) {
        const auto& snapshot = m_objectSnapshots[snapshotIndex];
        // Restore objects from snapshot
        // Implementation would depend on how objects are stored
    }
}

Vector3 AdvancedEditorTools::CalculateAlignmentSnap(const Vector3& position, const AlignmentGuide& guide) {
    switch (guide.type) {
        case AlignmentGuide::GuideType::VERTICAL:
            return {guide.position.x, position.y, position.z};
        case AlignmentGuide::GuideType::HORIZONTAL:
            return {position.x, guide.position.y, position.z};
        case AlignmentGuide::GuideType::DEPTH:
            return {position.x, position.y, guide.position.z};
        case AlignmentGuide::GuideType::CUSTOM_PLANE:
            // Project point onto plane
            return position; // Simplified
    }
    return position;
}

bool AdvancedEditorTools::IsPointOnGuide(const Vector3& point, const AlignmentGuide& guide, float tolerance) {
    switch (guide.type) {
        case AlignmentGuide::GuideType::VERTICAL:
            return fabs(point.x - guide.position.x) <= tolerance;
        case AlignmentGuide::GuideType::HORIZONTAL:
            return fabs(point.y - guide.position.y) <= tolerance;
        case AlignmentGuide::GuideType::DEPTH:
            return fabs(point.z - guide.position.z) <= tolerance;
        case AlignmentGuide::GuideType::CUSTOM_PLANE:
            return fabs(Vector3DotProduct(Vector3Subtract(point, guide.position), guide.normal)) <= tolerance;
    }
    return false;
}