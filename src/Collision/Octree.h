
//
// Created by Assistant - Octree collision system
#ifndef OCTREE_H
#define OCTREE_H

#include <limits>
#include <memory>
#include <raylib.h>
#include <raymath.h>
#include <vector>

// Forward declarations
struct OctreeNode;
struct CollisionTriangle;

//
// OctreeNode - represents a single node in the octree
//
struct OctreeNode
{
    Vector3 center;                           // Center of this node's bounding box
    float halfSize;                           // Half the size of this node's bounding box
    std::unique_ptr<OctreeNode> children[8];  // 8 child nodes (nullptr if leaf)
    std::vector<CollisionTriangle> triangles; // Triangles stored in this node
    bool isLeaf;                              // True if this is a leaf node

    OctreeNode(const Vector3 &center, float halfSize);

    // Get bounding box of this node
    Vector3 GetMin() const;
    Vector3 GetMax() const;

    // Check if point is inside this node
    bool Contains(const Vector3 &point) const;

    // Check if this node intersects with AABB
    bool IntersectsAABB(const Vector3 &min, const Vector3 &max) const;

    // Get child index for a point (0-7)
    int GetChildIndex(const Vector3 &point) const;
};

//
// Octree - spatial partitioning structure for efficient collision detection
//
class Octree
{
public:
    static constexpr int MAX_TRIANGLES_PER_NODE =
        20;                             // Reduced maximum triangles per leaf for better performance
    static constexpr int MAX_DEPTH = 8; // Reduced maximum tree depth for better performance
    static constexpr float MIN_NODE_SIZE = 2.0f; // Reduced minimum node size for better performance

    Octree();
    ~Octree() = default;

    // Initialize octree with bounding box
    void Initialize(const Vector3 &min, const Vector3 &max);

    // Build octree from model
    void BuildFromModel(Model *model, const Matrix &transform = MatrixIdentity());

    // Add triangle to octree
    void AddTriangle(const CollisionTriangle &triangle);

    // Clear all data
    void Clear();

    // Collision queries
    bool IntersectsAABB(const Vector3 &min, const Vector3 &max) const;
    bool IntersectsOctree(const Octree &other) const; // Precise octree-octree collision
    bool IntersectsImproved(const Vector3 &min,
                            const Vector3 &max) const; // Improved AABB collision
    bool ContainsPoint(const Vector3 &point) const;

    // Debug rendering
    void DebugDraw(const Color &color = RED) const;
    void DebugDrawRecursive(const OctreeNode *node, const Color &color) const;

    // Ray casting
    bool Raycast(const Vector3 &origin, const Vector3 &direction, float maxDistance,
                 float &hitDistance, Vector3 &hitPoint, Vector3 &hitNormal) const;

    // Debug information
    size_t GetTriangleCount() const;
    size_t GetNodeCount() const;
    int GetMaxDepth() const;
    size_t CountTriangles(const OctreeNode *node) const;

    // Debug rendering support
    void GetAllNodes(std::vector<std::pair<Vector3, float>> &nodes) const;

private:
    std::unique_ptr<OctreeNode> m_root;
    Vector3 m_min, m_max; // Bounding box of entire octree
    size_t m_triangleCount;

private:
    // Internal methods
    void BuildRecursive(OctreeNode *node, const std::vector<CollisionTriangle> &triangles,
                        int depth);
    void AddTriangleRecursive(OctreeNode *node, const CollisionTriangle &triangle, int depth);

    bool IntersectsAABBRecursive(const OctreeNode *node, const Vector3 &min,
                                 const Vector3 &max) const;
    bool IntersectsOctreeRecursive(const OctreeNode *thisNode, const OctreeNode *otherNode) const;
    bool IntersectsImprovedRecursive(const OctreeNode *node, const Vector3 &min,
                                     const Vector3 &max) const;
    bool ContainsPointRecursive(const OctreeNode *node, const Vector3 &point) const;
    bool RaycastRecursive(const OctreeNode *node, const Vector3 &origin, const Vector3 &direction,
                          float maxDistance, float &hitDistance, Vector3 &hitPoint,
                          Vector3 &hitNormal) const;

    void GetAllNodesRecursive(const OctreeNode *node,
                              std::vector<std::pair<Vector3, float>> &nodes) const;
    size_t CountNodesRecursive(const OctreeNode *node) const;

    // Helper methods
    bool TriangleIntersectsNode(const CollisionTriangle &triangle, const OctreeNode *node) const;
    bool TriangleIntersectsAABB(const CollisionTriangle &triangle, const Vector3 &boxMin,
                                const Vector3 &boxMax) const;
    void ExtractTrianglesFromModel(Model *model, const Matrix &transform,
                                   std::vector<CollisionTriangle> &triangles);
};

#endif // OCTREE_H