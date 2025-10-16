#include "CollisionSystem.h"
#include <cassert>
#include <cmath>
#include <cfloat>
#include <functional>
#include <future>
#include <thread>
#include <vector>
#include <algorithm>
#include <chrono>

#include "CollisionStructures.h"
#include <raylib.h>
#include <raymath.h>

// Initialize static cache
std::unordered_map<size_t, std::weak_ptr<Collision>> Collision::collisionCache;

Collision::Collision()
{
    m_min = {0, 0, 0};
    m_max = {0, 0, 0};
}
Collision::Collision(const Vector3 &center, const Vector3 &halfSize)
{
    m_min = Vector3Subtract(center, halfSize);
    m_max = Vector3Add(center, halfSize);
}

Collision::Collision(const Collision& other)
{
    m_min = other.m_min;
    m_max = other.m_max;
    m_collisionType = other.m_collisionType; 
    m_complexity = other.m_complexity;
    m_triangles = other.m_triangles;
    m_isBuilt = other.m_isBuilt;
    m_stats = other.m_stats;

    if (other.m_bvhRoot && other.IsUsingBVH()) {
        BuildBVHFromTriangles();
    }
}

Collision& Collision::operator=(const Collision& other)
{
    if (this == &other) return *this;
    
    m_min = other.m_min;
    m_max = other.m_max;
    m_collisionType = other.m_collisionType;
    m_complexity = other.m_complexity;
    m_triangles = other.m_triangles;
    m_isBuilt = other.m_isBuilt;
    m_stats = other.m_stats;


    if (!m_triangles.empty() && 
        (m_collisionType == CollisionType::BVH_ONLY || 
         m_collisionType == CollisionType::TRIANGLE_PRECISE))
    {
        BuildBVHFromTriangles();
    }
    
    return *this;
}

Collision::Collision(Collision &&other) noexcept
{
    m_min = other.m_min;
    m_max = other.m_max;
    m_collisionType = other.m_collisionType;
    m_complexity = other.m_complexity;
    m_triangles = std::move(other.m_triangles);
    m_bvhRoot = std::move(other.m_bvhRoot);
    m_isBuilt = other.m_isBuilt;
    m_stats = other.m_stats;
}
Collision &Collision::operator=(Collision &&other) noexcept
{
    if (this == &other)
        return *this;
    m_min = other.m_min;
    m_max = other.m_max;
    m_collisionType = other.m_collisionType;
    m_complexity = other.m_complexity;
    m_triangles = std::move(other.m_triangles);
    m_bvhRoot = std::move(other.m_bvhRoot);
    m_isBuilt = other.m_isBuilt;
    m_stats = other.m_stats;
    return *this;
}
Collision::~Collision() = default;

// ----------------- AABB -----------------
void Collision::Update(const Vector3 &center, const Vector3 &halfSize)
{
    m_min = Vector3Subtract(center, halfSize);
    m_max = Vector3Add(center, halfSize);
}

bool Collision::IntersectsAABB(const Collision &other) const
{
    if (m_max.x < other.m_min.x || m_min.x > other.m_max.x)
        return false;
    if (m_max.y < other.m_min.y || m_min.y > other.m_max.y)
        return false;
    if (m_max.z < other.m_min.z || m_min.z > other.m_max.z)
        return false;
    return true;
}

bool Collision::ContainsPointAABB(const Vector3 &point) const
{
    return (point.x >= m_min.x && point.x <= m_max.x) &&
           (point.y >= m_min.y && point.y <= m_max.y) && (point.z >= m_min.z && point.z <= m_max.z);
}

// ----------------- Build from model (optimized) -----------------
void Collision::BuildFromModel(void *model, const Matrix &transform)
{
    Model *rayModel = static_cast<Model *>(model);
    if (!rayModel)
    {
        TraceLog(LOG_ERROR, "Collision::BuildFromModel() - Invalid model pointer");
        return;
    }

    // Validate model data before processing
    if (rayModel->meshCount <= 0)
    {
        TraceLog(LOG_WARNING, "Collision::BuildFromModel() - Model has no meshes");
        return;
    }

    size_t totalTriangles = 0;
    bool hasValidMeshes = false;

    for (int meshIdx = 0; meshIdx < rayModel->meshCount; ++meshIdx)
    {
        Mesh &mesh = rayModel->meshes[meshIdx];
        if (!mesh.vertices || !mesh.indices)
        {
            TraceLog(LOG_WARNING, "Collision::BuildFromModel() - Mesh %d has null vertex or index data", meshIdx);
            continue;
        }

        if (mesh.vertexCount == 0 || mesh.triangleCount == 0)
        {
            TraceLog(LOG_WARNING, "Collision::BuildFromModel() - Mesh %d has no vertices or triangles", meshIdx);
            continue;
        }

        // Validate mesh data sizes
        if (mesh.vertexCount > 1000000) // Sanity check for vertex count
        {
            TraceLog(LOG_ERROR, "Collision::BuildFromModel() - Mesh %d has excessive vertex count (%d)", meshIdx, mesh.vertexCount);
            continue;
        }

        if (mesh.triangleCount > 1000000) // Sanity check for triangle count
        {
            TraceLog(LOG_ERROR, "Collision::BuildFromModel() - Mesh %d has excessive triangle count (%d)", meshIdx, mesh.triangleCount);
            continue;
        }

        totalTriangles += mesh.triangleCount;
        hasValidMeshes = true;
    }

    if (!hasValidMeshes)
    {
        TraceLog(LOG_ERROR, "Collision::BuildFromModel() - No valid meshes found in model");
        return;
    }

    if (totalTriangles == 0)
    {
        TraceLog(LOG_WARNING, "Collision::BuildFromModel() - No triangles found in model");
        return;
    }

    if (totalTriangles > 1000000) // Sanity check for total triangles
    {
        TraceLog(LOG_ERROR, "Collision::BuildFromModel() - Model has excessive triangle count (%zu)", totalTriangles);
        return;
    }

    // Pre-allocate memory for better performance
    try
    {
        m_triangles.reserve(totalTriangles);
    }
    catch (const std::exception& e)
    {
        TraceLog(LOG_ERROR, "Collision::BuildFromModel() - Failed to reserve memory for triangles: %s", e.what());
        return;
    }

    // Process all meshes and collect vertices for batch transformation
    for (int meshIdx = 0; meshIdx < rayModel->meshCount; ++meshIdx)
    {
        Mesh &mesh = rayModel->meshes[meshIdx];

        // Enhanced validation for mesh data
        if (!mesh.vertices || !mesh.indices)
        {
            TraceLog(LOG_WARNING, "Mesh %d has null vertex or index data, skipping", meshIdx);
            continue;
        }

        if (mesh.vertexCount == 0 || mesh.triangleCount == 0)
        {
            TraceLog(LOG_WARNING, "Mesh %d has no vertices or triangles, skipping", meshIdx);
            continue;
        }

        // Validate mesh data integrity
        size_t expectedVertexDataSize = mesh.vertexCount * 3 * sizeof(float);
        size_t expectedIndexDataSize = mesh.triangleCount * 3 * sizeof(unsigned short);

        if (mesh.vertices == nullptr || mesh.indices == nullptr)
        {
            TraceLog(LOG_ERROR, "Mesh %d: Invalid mesh data pointers", meshIdx);
            continue;
        }

        // Batch process triangles for this mesh
        const int triangleCount = mesh.triangleCount;
        const unsigned short* indices = mesh.indices;

        for (int i = 0; i < triangleCount; ++i)
        {
            const int idx = i * 3;

            // Bounds check for indices
            if (idx + 2 >= mesh.triangleCount * 3)
            {
                TraceLog(LOG_ERROR, "Mesh %d: Index out of bounds at triangle %d", meshIdx, i);
                break;
            }

            const int i0 = indices[idx + 0];
            const int i1 = indices[idx + 1];
            const int i2 = indices[idx + 2];

            // Validate vertex indices are within bounds
            if (i0 >= mesh.vertexCount || i1 >= mesh.vertexCount || i2 >= mesh.vertexCount)
            {
                TraceLog(LOG_ERROR, "Mesh %d: Vertex index out of bounds (i0=%d, i1=%d, i2=%d, vertexCount=%d)",
                         meshIdx, i0, i1, i2, mesh.vertexCount);
                continue;
            }

            // Get vertex positions with bounds checking
            const float* v0Ptr = &mesh.vertices[i0 * 3];
            const float* v1Ptr = &mesh.vertices[i1 * 3];
            const float* v2Ptr = &mesh.vertices[i2 * 3];

            // Validate vertex data pointers
            if (v0Ptr < mesh.vertices || v0Ptr >= mesh.vertices + (mesh.vertexCount * 3) ||
                v1Ptr < mesh.vertices || v1Ptr >= mesh.vertices + (mesh.vertexCount * 3) ||
                v2Ptr < mesh.vertices || v2Ptr >= mesh.vertices + (mesh.vertexCount * 3))
            {
                TraceLog(LOG_ERROR, "Mesh %d: Vertex data pointer out of bounds", meshIdx);
                continue;
            }

            Vector3 v0 = { v0Ptr[0], v0Ptr[1], v0Ptr[2] };
            Vector3 v1 = { v1Ptr[0], v1Ptr[1], v1Ptr[2] };
            Vector3 v2 = { v2Ptr[0], v2Ptr[1], v2Ptr[2] };

            // Validate vertex data is finite (not NaN or inf)
            if (!std::isfinite(v0.x) || !std::isfinite(v0.y) || !std::isfinite(v0.z) ||
                !std::isfinite(v1.x) || !std::isfinite(v1.y) || !std::isfinite(v1.z) ||
                !std::isfinite(v2.x) || !std::isfinite(v2.y) || !std::isfinite(v2.z))
            {
                TraceLog(LOG_WARNING, "Mesh %d: Invalid vertex data (NaN or inf) at triangle %d", meshIdx, i);
                continue;
            }

            // Check for degenerate triangles (zero area)
            Vector3 edge1 = Vector3Subtract(v1, v0);
            Vector3 edge2 = Vector3Subtract(v2, v0);
            Vector3 normal = Vector3CrossProduct(edge1, edge2);
            float area = Vector3Length(normal);

            if (area < 1e-12f)
            {
                TraceLog(LOG_DEBUG, "Mesh %d: Degenerate triangle %d (area too small)", meshIdx, i);
                continue;
            }

            // Transform vertices to world coordinates
            v0 = Vector3Transform(v0, transform);
            v1 = Vector3Transform(v1, transform);
            v2 = Vector3Transform(v2, transform);

            // Validate transformed vertices
            if (!std::isfinite(v0.x) || !std::isfinite(v0.y) || !std::isfinite(v0.z) ||
                !std::isfinite(v1.x) || !std::isfinite(v1.y) || !std::isfinite(v1.z) ||
                !std::isfinite(v2.x) || !std::isfinite(v2.y) || !std::isfinite(v2.z))
            {
                TraceLog(LOG_WARNING, "Mesh %d: Invalid transformed vertex data at triangle %d", meshIdx, i);
                continue;
            }

            // Use emplace_back for better performance (no copy)
            try
            {
                m_triangles.emplace_back(v0, v1, v2);
            }
            catch (const std::exception& e)
            {
                TraceLog(LOG_ERROR, "Mesh %d: Failed to add triangle %d: %s", meshIdx, i, e.what());
                continue;
            }
        }
    }

    TraceLog(LOG_INFO, "Collision triangles: %zu", m_triangles.size());

    // Build AABB and BVH only if we have triangles
    if (!m_triangles.empty())
    {
        UpdateAABBFromTriangles();
        BuildBVHFromTriangles();
    }
    else
    {
        // Fallback to model's bounding box if no triangles
        BoundingBox bb = GetModelBoundingBox(*rayModel);
        Vector3 corners[8] = {
            {bb.min.x, bb.min.y, bb.min.z}, {bb.max.x, bb.min.y, bb.min.z},
            {bb.min.x, bb.max.y, bb.min.z}, {bb.min.x, bb.min.y, bb.max.z},
            {bb.max.x, bb.max.y, bb.min.z}, {bb.min.x, bb.max.y, bb.max.z},
            {bb.max.x, bb.min.y, bb.max.z}, {bb.max.x, bb.max.y, bb.max.z}
        };

        Vector3 tmin = {FLT_MAX, FLT_MAX, FLT_MAX};
        Vector3 tmax = {-FLT_MAX, -FLT_MAX, -FLT_MAX};

        for (const auto &corner : corners)
        {
            Vector3 tc = Vector3Transform(corner, transform);
            tmin.x = fminf(tmin.x, tc.x); tmin.y = fminf(tmin.y, tc.y); tmin.z = fminf(tmin.z, tc.z);
            tmax.x = fmaxf(tmax.x, tc.x); tmax.y = fmaxf(tmax.y, tc.y); tmax.z = fmaxf(tmax.z, tc.z);
        }

        m_min = tmin;
        m_max = tmax;
    }

    m_isBuilt = true;
}

// Structure to hold mesh processing data for parallel processing
struct MeshProcessingData
{
    std::vector<CollisionTriangle> triangles;
    int meshIndex;
};

// Function to process a single mesh in parallel
MeshProcessingData ProcessMeshParallel(const Mesh& mesh, const Matrix& transform, int meshIndex)
{
    MeshProcessingData result;
    result.meshIndex = meshIndex;
    result.triangles.reserve(mesh.triangleCount);

    if (!mesh.vertices || !mesh.indices) {
        return result;
    }

    // Batch process triangles for this mesh
    const int triangleCount = mesh.triangleCount;
    const unsigned short* indices = mesh.indices;

    for (int i = 0; i < triangleCount; ++i)
    {
        const int idx = i * 3;

        // Bounds check for indices
        if (idx + 2 >= mesh.triangleCount * 3)
        {
            TraceLog(LOG_ERROR, "ProcessMeshParallel: Index out of bounds at triangle %d", i);
            break;
        }

        const int i0 = indices[idx + 0];
        const int i1 = indices[idx + 1];
        const int i2 = indices[idx + 2];

        // Validate vertex indices are within bounds
        if (i0 >= mesh.vertexCount || i1 >= mesh.vertexCount || i2 >= mesh.vertexCount)
        {
            TraceLog(LOG_ERROR, "ProcessMeshParallel: Vertex index out of bounds (i0=%d, i1=%d, i2=%d, vertexCount=%d)",
                     i0, i1, i2, mesh.vertexCount);
            continue;
        }

        // Get vertex positions with bounds checking
        const float* v0Ptr = &mesh.vertices[i0 * 3];
        const float* v1Ptr = &mesh.vertices[i1 * 3];
        const float* v2Ptr = &mesh.vertices[i2 * 3];

        // Validate vertex data pointers
        if (v0Ptr < mesh.vertices || v0Ptr >= mesh.vertices + (mesh.vertexCount * 3) ||
            v1Ptr < mesh.vertices || v1Ptr >= mesh.vertices + (mesh.vertexCount * 3) ||
            v2Ptr < mesh.vertices || v2Ptr >= mesh.vertices + (mesh.vertexCount * 3))
        {
            TraceLog(LOG_ERROR, "ProcessMeshParallel: Vertex data pointer out of bounds");
            continue;
        }

        Vector3 v0 = { v0Ptr[0], v0Ptr[1], v0Ptr[2] };
        Vector3 v1 = { v1Ptr[0], v1Ptr[1], v1Ptr[2] };
        Vector3 v2 = { v2Ptr[0], v2Ptr[1], v2Ptr[2] };

        // Validate vertex data is finite (not NaN or inf)
        if (!std::isfinite(v0.x) || !std::isfinite(v0.y) || !std::isfinite(v0.z) ||
            !std::isfinite(v1.x) || !std::isfinite(v1.y) || !std::isfinite(v1.z) ||
            !std::isfinite(v2.x) || !std::isfinite(v2.y) || !std::isfinite(v2.z))
        {
            TraceLog(LOG_WARNING, "ProcessMeshParallel: Invalid vertex data (NaN or inf) at triangle %d", i);
            continue;
        }

        // Transform vertices to world coordinates
        v0 = Vector3Transform(v0, transform);
        v1 = Vector3Transform(v1, transform);
        v2 = Vector3Transform(v2, transform);

        // Use emplace_back for better performance (no copy)
        result.triangles.emplace_back(v0, v1, v2);
    }

    return result;
}

// Parallel version of BuildFromModel for better performance with complex models
void Collision::BuildFromModelParallel(void *model, const Matrix &transform)
{
    Model *rayModel = static_cast<Model *>(model);
    if (!rayModel) return;

    // Count total triangles across all meshes
    size_t totalTriangles = 0;
    for (int meshIdx = 0; meshIdx < rayModel->meshCount; ++meshIdx)
    {
        Mesh &mesh = rayModel->meshes[meshIdx];
        if (!mesh.vertices || !mesh.indices) continue;
        totalTriangles += mesh.triangleCount;
    }

    // Pre-allocate memory for better performance
    m_triangles.reserve(totalTriangles);

    // Determine optimal number of threads (leave some cores free for other tasks)
    unsigned int numThreads = std::min(static_cast<unsigned int>(rayModel->meshCount),
                                      std::max(1u, std::thread::hardware_concurrency() / 2));

    if (numThreads == 1 || rayModel->meshCount <= 1)
    {
        // Fall back to sequential processing for simple cases
        BuildFromModel(model, transform);
        return;
    }

    // Launch mesh processing tasks in parallel
    std::vector<std::future<MeshProcessingData>> futures;
    std::vector<MeshProcessingData> meshResults;

    for (int meshIdx = 0; meshIdx < rayModel->meshCount; ++meshIdx)
    {
        Mesh &mesh = rayModel->meshes[meshIdx];
        if (!mesh.vertices || !mesh.indices) continue;

        // Distribute tasks across available threads
        if (futures.size() < numThreads)
        {
            futures.push_back(std::async(std::launch::async, ProcessMeshParallel,
                                       std::ref(mesh), transform, meshIdx));
        }
        else
        {
            // Wait for one task to complete before starting another
            meshResults.push_back(futures.back().get());
            futures.pop_back();
            futures.push_back(std::async(std::launch::async, ProcessMeshParallel,
                                       std::ref(mesh), transform, meshIdx));
        }
    }

    // Collect remaining results
    for (auto& future : futures)
    {
        meshResults.push_back(future.get());
    }

    // Combine all mesh results into single triangle list
    for (auto& meshData : meshResults)
    {
        m_triangles.insert(m_triangles.end(),
                          std::make_move_iterator(meshData.triangles.begin()),
                          std::make_move_iterator(meshData.triangles.end()));
    }

    TraceLog(LOG_INFO, "Collision triangles (parallel): %zu", m_triangles.size());

    // Build AABB and BVH only if we have triangles
    if (!m_triangles.empty())
    {
        UpdateAABBFromTriangles();
        BuildBVHFromTriangles();
    }
    else
    {
        // Fallback to model's bounding box if no triangles
        BoundingBox bb = GetModelBoundingBox(*rayModel);
        Vector3 corners[8] = {
            {bb.min.x, bb.min.y, bb.min.z}, {bb.max.x, bb.min.y, bb.min.z},
            {bb.min.x, bb.max.y, bb.min.z}, {bb.min.x, bb.min.y, bb.max.z},
            {bb.max.x, bb.max.y, bb.min.z}, {bb.min.x, bb.max.y, bb.max.z},
            {bb.max.x, bb.min.y, bb.max.z}, {bb.max.x, bb.max.y, bb.max.z}
        };

        Vector3 tmin = {FLT_MAX, FLT_MAX, FLT_MAX};
        Vector3 tmax = {-FLT_MAX, -FLT_MAX, -FLT_MAX};

        for (const auto &corner : corners)
        {
            Vector3 tc = Vector3Transform(corner, transform);
            tmin.x = fminf(tmin.x, tc.x); tmin.y = fminf(tmin.y, tc.y); tmin.z = fminf(tmin.z, tc.z);
            tmax.x = fmaxf(tmax.x, tc.x); tmax.y = fmaxf(tmax.y, tc.y); tmax.z = fmaxf(tmax.z, tc.z);
        }

        m_min = tmin;
        m_max = tmax;
    }

    m_isBuilt = true;
}

void Collision::BuildFromModelWithType(void *model, CollisionType type, const Matrix &transform)
{
    m_collisionType = type;
    BuildFromModel(model, transform);
}

void Collision::CalculateFromModel(void *model, const Matrix &transform)
{
    BuildFromModel(model, transform);
}

// ----------------- Helpers for triangle bounds -----------------
void Collision::ExpandAABB(Vector3 &minOut, Vector3 &maxOut, const Vector3 &p)
{
    minOut.x = std::min(minOut.x, p.x);
    minOut.y = std::min(minOut.y, p.y);
    minOut.z = std::min(minOut.z, p.z);
    maxOut.x = std::max(maxOut.x, p.x);
    maxOut.y = std::max(maxOut.y, p.y);
    maxOut.z = std::max(maxOut.z, p.z);
}

void Collision::TriangleBounds(const CollisionTriangle &t, Vector3 &outMin, Vector3 &outMax)
{
    outMin = t.V0();
    outMax = t.V0();
    ExpandAABB(outMin, outMax, t.V1());
    ExpandAABB(outMin, outMax, t.V2());
}

void Collision::UpdateAABBFromTriangles()
{
    if (m_triangles.empty())
        return;

    // Initialize with first triangle vertices
    const CollisionTriangle& firstTri = m_triangles[0];
    Vector3 minP = firstTri.V0();
    Vector3 maxP = firstTri.V0();

    // Unroll first few iterations for better performance
    ExpandAABB(minP, maxP, firstTri.V1());
    ExpandAABB(minP, maxP, firstTri.V2());

    // Process remaining triangles in optimized loop
    const size_t triangleCount = m_triangles.size();
    for (size_t i = 1; i < triangleCount; ++i)
    {
        const CollisionTriangle& t = m_triangles[i];
        // Process all three vertices at once for better cache performance
        ExpandAABB(minP, maxP, t.V0());
        ExpandAABB(minP, maxP, t.V1());
        ExpandAABB(minP, maxP, t.V2());
    }

    m_min = minP;
    m_max = maxP;
}

void Collision::AddTriangle(const CollisionTriangle &triangle)
{
    m_triangles.push_back(triangle);
}

void Collision::AddTriangles(const std::vector<CollisionTriangle> &triangles)
{
    m_triangles.insert(m_triangles.end(), triangles.begin(), triangles.end());
}

// ----------------- BVH build -----------------
static int const MAX_TRIANGLES_PER_LEAF = 8;
static int const MAX_BVH_DEPTH = 90;

std::unique_ptr<BVHNode> Collision::BuildBVHNode(std::vector<CollisionTriangle> &tris, int depth)
{
    auto node = std::make_unique<BVHNode>();
    // compute bounds
    if (tris.empty())
        return node;
    Vector3 minB = tris[0].V0();
    Vector3 maxB = minB;
    for (auto &t : tris)
    {
        ExpandAABB(minB, maxB, t.V0());
        ExpandAABB(minB, maxB, t.V1());
        ExpandAABB(minB, maxB, t.V2());
    }
    node->min = minB;
    node->max = maxB;

    if ((int)tris.size() <= MAX_TRIANGLES_PER_LEAF || depth >= MAX_BVH_DEPTH)
    {
        node->triangles = std::move(tris); // leaf - move to avoid copy
        return node;
    }

    // choose split axis by longest axis
    Vector3 ext = Vector3Subtract(maxB, minB);
    int axis = 0;
    if (ext.y > ext.x && ext.y >= ext.z)
        axis = 1;
    else if (ext.z > ext.x && ext.z > ext.y)
        axis = 2;

    // sort by centroid along axis (use faster sorting with safety checks)
    std::sort(tris.begin(), tris.end(),
              [axis](const CollisionTriangle &a, const CollisionTriangle &b)
              {
                  // Safely calculate centroids with finite value checks
                  Vector3 ca = Vector3Add(Vector3Add(a.V0(), a.V1()), a.V2());
                  Vector3 cb = Vector3Add(Vector3Add(b.V0(), b.V1()), b.V2());

                  float ca_val = 0.0f, cb_val = 0.0f;

                  switch (axis)
                  {
                  case 0:
                      ca_val = ca.x / 3.0f;
                      cb_val = cb.x / 3.0f;
                      break;
                  case 1:
                      ca_val = ca.y / 3.0f;
                      cb_val = cb.y / 3.0f;
                      break;
                  case 2:
                      ca_val = ca.z / 3.0f;
                      cb_val = cb.z / 3.0f;
                      break;
                  default:
                      ca_val = ca.x / 3.0f;
                      cb_val = cb.x / 3.0f;
                      break;
                  }

                  // Handle NaN/inf values
                  if (!std::isfinite(ca_val)) ca_val = 0.0f;
                  if (!std::isfinite(cb_val)) cb_val = 0.0f;

                  return ca_val < cb_val;
              });

    size_t mid = tris.size() / 2;
    std::vector<CollisionTriangle> leftTris(tris.begin(), tris.begin() + mid);
    std::vector<CollisionTriangle> rightTris(tris.begin() + mid, tris.end());

    node->left = BuildBVHNode(leftTris, depth + 1);
    node->right = BuildBVHNode(rightTris, depth + 1);

    return node;
}

void Collision::BuildBVHFromTriangles()
{
    if (m_triangles.empty())
    {
        TraceLog(LOG_DEBUG, "Collision::BuildBVHFromTriangles() - No triangles to build BVH");
        m_bvhRoot.reset();
        return;
    }

    // Validate triangles before building BVH
    size_t validTriangles = 0;
    for (const auto& tri : m_triangles)
    {
        if (std::isfinite(tri.V0().x) && std::isfinite(tri.V0().y) && std::isfinite(tri.V0().z) &&
            std::isfinite(tri.V1().x) && std::isfinite(tri.V1().y) && std::isfinite(tri.V1().z) &&
            std::isfinite(tri.V2().x) && std::isfinite(tri.V2().y) && std::isfinite(tri.V2().z))
        {
            validTriangles++;
        }
    }

    if (validTriangles == 0)
    {
        TraceLog(LOG_ERROR, "Collision::BuildBVHFromTriangles() - No valid triangles found");
        m_bvhRoot.reset();
        return;
    }

    if (validTriangles < m_triangles.size())
    {
        TraceLog(LOG_WARNING, "Collision::BuildBVHFromTriangles() - Found %zu invalid triangles out of %zu total",
                 m_triangles.size() - validTriangles, m_triangles.size());
    }

    // Build BVH directly with existing triangles - no copying needed
    try
    {
        m_bvhRoot = BuildBVHNode(m_triangles, 0);
        TraceLog(LOG_INFO, "Collision::BuildBVHFromTriangles() - Successfully built BVH with %zu triangles", validTriangles);
    }
    catch (const std::exception& e)
    {
        TraceLog(LOG_ERROR, "Collision::BuildBVHFromTriangles() - Failed to build BVH: %s", e.what());
        m_bvhRoot.reset();
    }
}

// ----------------- Ray/triangle (Möller–Trumbore) -----------------
bool Collision::RayIntersectsTriangle(const Vector3 &orig, const Vector3 &dir,
                                       const CollisionTriangle &tri, RayHit &outHit)
{
    // Enhanced Möller-Trumbore with safety checks
    const float EPS_PARALLEL = 1e-8f;
    Vector3 edge1 = Vector3Subtract(tri.V1(), tri.V0());
    Vector3 edge2 = Vector3Subtract(tri.V2(), tri.V0());

    // Check for degenerate triangles
    if (Vector3LengthSqr(edge1) < 1e-12f || Vector3LengthSqr(edge2) < 1e-12f)
        return false;

    Vector3 h = Vector3CrossProduct(dir, edge2);
    float a = Vector3DotProduct(edge1, h);

    // Enhanced parallel check
    if (fabsf(a) < EPS_PARALLEL)
        return false; // parallel

    float f = 1.0f / a;

    // Check for invalid division result
    if (!std::isfinite(f))
        return false;

    Vector3 s = Vector3Subtract(orig, tri.V0());
    float u = f * Vector3DotProduct(s, h);

    if (u < 0.0f || u > 1.0f)
        return false;

    Vector3 q = Vector3CrossProduct(s, edge1);
    float v = f * Vector3DotProduct(dir, q);

    if (v < 0.0f || u + v > 1.0f)
        return false;

    float t = f * Vector3DotProduct(edge2, q);

    // Check for valid intersection distance and finite values
    if (std::isfinite(t) && t > 1e-6f)
    {
        outHit.hit = true;
        outHit.distance = t;
        outHit.position = Vector3Add(orig, Vector3Scale(dir, t));

        // Safely compute normal
        Vector3 normal = Vector3CrossProduct(edge1, edge2);
        float normalLengthSqr = Vector3LengthSqr(normal);
        if (normalLengthSqr > 1e-12f)
        {
            outHit.normal = Vector3Scale(normal, 1.0f / sqrtf(normalLengthSqr));
        }
        else
        {
            outHit.normal = {0.0f, 1.0f, 0.0f}; // Fallback normal
        }
        return true;
    }
    return false;
}

// ----------------- AABB ray test (slab) -----------------
bool Collision::AABBIntersectRay(const Vector3 &min, const Vector3 &max, const Vector3 &origin,
                                 const Vector3 &dir, float maxDistance)
{
    float tmin = 0.0f;
    float tmax = maxDistance;

    for (int i = 0; i < 3; ++i)
    {
        float d = (i == 0) ? dir.x : (i == 1 ? dir.y : dir.z);
        float o = (i == 0) ? origin.x : (i == 1 ? origin.y : origin.z);
        float mn = (i == 0) ? min.x : (i == 1 ? min.y : min.z);
        float mx = (i == 0) ? max.x : (i == 1 ? max.y : max.z);

        if (fabsf(d) < 1e-8f)
        {
            // Ray is parallel to slab; reject if origin not within slab
            if (o < mn || o > mx)
                return false;
            // Otherwise, slab does not constrain t
            continue;
        }

        float invD = 1.0f / d;
        float t0 = (mn - o) * invD;
        float t1 = (mx - o) * invD;
        if (invD < 0.0f)
            std::swap(t0, t1);
        tmin = std::max(tmin, t0);
        tmax = std::min(tmax, t1);
        if (tmax < tmin)
            return false;
    }
    return true;
}

// ----------------- BVH raycast traversal -----------------
bool Collision::RaycastBVHNode(const BVHNode *node, const Vector3 &origin, const Vector3 &dir,
                               float maxDistance, RayHit &outHit) const
{
    if (!node)
        return false;
    if (!AABBIntersectRay(node->min, node->max, origin, dir, maxDistance))
        return false;

    bool hitAny = false;
    if (node->IsLeaf())
    {
        // Batch process triangles in leaf nodes for better cache performance
        const size_t triangleCount = node->triangles.size();
        for (size_t i = 0; i < triangleCount; ++i)
        {
            const auto &tri = node->triangles[i];
            RayHit hit;
            if (RayIntersectsTriangle(origin, dir, tri, hit))
            {
                if (hit.distance < outHit.distance)
                {
                    outHit = hit;
                    hitAny = true;
                }
            }
        }
        return hitAny;
    }

    // traverse both children (order heuristic could be added)
    bool hitL = RaycastBVHNode(node->left.get(), origin, dir, maxDistance, outHit);
    bool hitR = RaycastBVHNode(node->right.get(), origin, dir, maxDistance, outHit);
    return hitL || hitR;
}

bool Collision::RaycastBVH(const Vector3 &origin, const Vector3 &dir, float maxDistance,
                            RayHit &outHit) const
{
    if (!m_bvhRoot)
        return false;

    outHit.hit = false;
    outHit.distance = std::numeric_limits<float>::infinity();

    // Safely normalize direction vector
    float dirLengthSqr = Vector3LengthSqr(dir);
    if (dirLengthSqr < 1e-12f)
        return false; // Invalid direction vector

    Vector3 ndir = Vector3Scale(dir, 1.0f / sqrtf(dirLengthSqr));
    bool ok = RaycastBVHNode(m_bvhRoot.get(), origin, ndir, maxDistance, outHit);
    return ok;
}

// ----------------- Point-in-mesh via raycast (count intersections) -----------------
bool Collision::ContainsPointBVH(const Vector3 &point) const
{
    // Cast ray in +X direction and count intersections
    Vector3 dir = Vector3Normalize((Vector3){1.0f, 0.0001f, 0.0002f}); // slight offset to avoid coplanar degeneracy
    RayHit hit;
    int count = 0;
    const float MAX_DIST = 1e6f;

    // Instead of single Raycast that returns nearest, we need to count all intersections along ray.
    // Simple way: traverse BVH and for each triangle test ray intersection and count t>EPS.
    if (!m_bvhRoot)
        return false;

    // Flatten traversal - naive approach: iterate all triangles (costly) OR
    // write a specialized traversal that collects all hits. For simplicity, we do traversal and
    // check. We'll implement a stackless recursion using lambda:
    std::vector<const BVHNode *> stack;
    stack.push_back(m_bvhRoot.get());
    while (!stack.empty())
    {
        const BVHNode *n = stack.back();
        stack.pop_back();
        if (!AABBIntersectRay(n->min, n->max, point, dir, MAX_DIST))
            continue;
        if (n->IsLeaf())
        {
            for (const auto &tri : n->triangles)
            {
                RayHit rh;
                if (RayIntersectsTriangle(point, dir, tri, rh))
                {
                    if (rh.distance > 1e-6f)
                        count++;
                }
            }
        }
        else
        {
            if (n->left)
                stack.push_back(n->left.get());
            if (n->right)
                stack.push_back(n->right.get());
        }
    }
    return (count % 2) == 1;
}

// ----------------- Intersects (AABB broad, BVH narrow) -----------------
// Helper: check if other's BVH has any leaf AABB overlapping this Collision's AABB
// Optimized Triangle-AABB SAT test with early exit optimizations
static bool TriangleAABBOverlapSAT(const CollisionTriangle &tri, const Vector3 &bmin, const Vector3 &bmax)
{
    // Quick AABB-AABB test first (much faster than full SAT)
    Vector3 triMin = tri.V0();
    Vector3 triMax = tri.V0();

    // Calculate triangle AABB inline for speed
    if (tri.V1().x < triMin.x) triMin.x = tri.V1().x; else if (tri.V1().x > triMax.x) triMax.x = tri.V1().x;
    if (tri.V1().y < triMin.y) triMin.y = tri.V1().y; else if (tri.V1().y > triMax.y) triMax.y = tri.V1().y;
    if (tri.V1().z < triMin.z) triMin.z = tri.V1().z; else if (tri.V1().z > triMax.z) triMax.z = tri.V1().z;

    if (tri.V2().x < triMin.x) triMin.x = tri.V2().x; else if (tri.V2().x > triMax.x) triMax.x = tri.V2().x;
    if (tri.V2().y < triMin.y) triMin.y = tri.V2().y; else if (tri.V2().y > triMax.y) triMax.y = tri.V2().y;
    if (tri.V2().z < triMin.z) triMin.z = tri.V2().z; else if (tri.V2().z > triMax.z) triMax.z = tri.V2().z;

    // Early exit: if triangle AABB doesn't overlap box AABB, no intersection
    if (triMax.x < bmin.x || triMin.x > bmax.x) return false;
    if (triMax.y < bmin.y || triMin.y > bmax.y) return false;
    if (triMax.z < bmin.z || triMin.z > bmax.z) return false;

    // If we get here, AABBs overlap, so we need full SAT test
    // Centered box with half extents
    Vector3 c = { (bmin.x + bmax.x) * 0.5f, (bmin.y + bmax.y) * 0.5f, (bmin.z + bmax.z) * 0.5f };
    Vector3 h = { (bmax.x - bmin.x) * 0.5f, (bmax.y - bmin.y) * 0.5f, (bmax.z - bmin.z) * 0.5f };

    // Triangle vertices relative to box center
    Vector3 v0 = Vector3Subtract(tri.V0(), c);
    Vector3 v1 = Vector3Subtract(tri.V1(), c);
    Vector3 v2 = Vector3Subtract(tri.V2(), c);

    // Box axes
    const Vector3 ax = {1.0f, 0.0f, 0.0f};
    const Vector3 ay = {0.0f, 1.0f, 0.0f};
    const Vector3 az = {0.0f, 0.0f, 1.0f};

    auto axisTest = [&](const Vector3 &axis) -> bool
    {
        const float len = Vector3Length(axis);
        if (len < 1e-8f)
            return true; // skip near-zero axis
        Vector3 n = Vector3Scale(axis, 1.0f / len);
        const float p0 = Vector3DotProduct(v0, n);
        const float p1 = Vector3DotProduct(v1, n);
        const float p2 = Vector3DotProduct(v2, n);
        const float triMin = fminf(p0, fminf(p1, p2));
        const float triMax = fmaxf(p0, fmaxf(p1, p2));
        const float r = h.x * fabsf(n.x) + h.y * fabsf(n.y) + h.z * fabsf(n.z);
        return !(triMin > r || triMax < -r);
    };

    // 1) Test box axes (most likely to fail first)
    if (!axisTest(ax)) return false;
    if (!axisTest(ay)) return false;
    if (!axisTest(az)) return false;

    // 2) Test triangle plane normal
    Vector3 e0 = Vector3Subtract(v1, v0);
    Vector3 e1 = Vector3Subtract(v2, v0);
    Vector3 n = Vector3CrossProduct(e0, e1);
    if (!axisTest(n)) return false;

    // 3) Test cross products of triangle edges with box axes (9 axes)
    Vector3 e2 = Vector3Subtract(v2, v1);
    const Vector3 edges[3] = { e0, e1, e2 };
    const Vector3 boxAxes[3] = { ax, ay, az };
    for (const Vector3 &e : edges)
    {
        for (const Vector3 &ba : boxAxes)
        {
            Vector3 a = Vector3CrossProduct(e, ba);
            if (!axisTest(a)) return false;
        }
    }

    return true; // no separating axis found
}

static bool BVHOverlapsAABB(const BVHNode* node, const Collision& aabbCollider)
{
    if (!node) return false;
    // Quick reject: node AABB vs collider AABB
    Collision nodeBox{ Vector3Scale(Vector3Add(node->min, node->max), 0.5f),
                       Vector3Scale(Vector3Subtract(node->max, node->min), 0.5f) };
    if (!nodeBox.IntersectsAABB(aabbCollider))
        return false;
    if (node->IsLeaf())
    {
        // Check each triangle's AABB against collider AABB for a tighter test
        Vector3 otherMin = aabbCollider.GetMin();
        Vector3 otherMax = aabbCollider.GetMax();
        for (const auto &tri : node->triangles)
        {
            if (TriangleAABBOverlapSAT(tri, otherMin, otherMax))
            {
                return true;
            }
        }
        return false;
    }
    return BVHOverlapsAABB(node->left.get(), aabbCollider) ||
           BVHOverlapsAABB(node->right.get(), aabbCollider);
}

bool Collision::Intersects(const Collision &other) const
{
    // Broad-phase AABB
    if (!IntersectsAABB(other))
        return false;

    const bool thisHasBVH = (m_bvhRoot != nullptr);
    const bool otherHasBVH = (other.m_bvhRoot != nullptr);

    // If both have BVH: use BVH node-vs-AABB overlap as a better narrow-phase
    if (thisHasBVH && otherHasBVH)
    {
        // Check if any of other's leaf nodes overlaps this AABB and vice versa
        bool otherIntoThis = BVHOverlapsAABB(other.m_bvhRoot.get(), *this);
        if (!otherIntoThis) return false;
        bool thisIntoOther = BVHOverlapsAABB(m_bvhRoot.get(), other);
        return thisIntoOther;
    }

    // If only other has BVH, ensure some BVH leaf overlaps our AABB; otherwise reject
    if (!thisHasBVH && otherHasBVH)
    {
        return BVHOverlapsAABB(other.m_bvhRoot.get(), *this);
    }

    // If only this has BVH, mirror the test
    if (thisHasBVH && !otherHasBVH)
    {
        return BVHOverlapsAABB(m_bvhRoot.get(), other);
    }

    // No BVH on either side: AABB overlap is our best answer
    return true;
}

// ======================================================================================================================
void Collision::SetCollisionType(CollisionType type) { m_collisionType = type; }
bool BVHNode::IsLeaf() const { return !left && !right; }
Vector3 Collision::GetSize() const
{
    return Vector3Subtract(m_max, m_min);
}
Vector3 Collision::GetCenter() const
{
    return Vector3Scale(Vector3Add(m_min, m_max), 0.5f);
}
Vector3 Collision::GetMax() const { return m_max; }
Vector3 Collision::GetMin() const { return m_min; }
size_t Collision::GetTriangleCount() const { return m_triangles.size(); }
bool Collision::HasTriangleData() const { return !m_triangles.empty(); }
const Collision::PerformanceStats &Collision::GetPerformanceStats() const { return m_stats; }

// Compatibility: map Octree-style raycast to BVH
bool Collision::RaycastOctree(const Vector3 &origin, const Vector3 &dir, float maxDistance,
                              float &hitDistance, Vector3 &hitPoint, Vector3 &hitNormal) const
{
    RayHit hit;
    if (!RaycastBVH(origin, dir, maxDistance, hit))
        return false;
    hitDistance = hit.distance;
    hitPoint = hit.position;
    hitNormal = hit.normal;
    return true;
}
CollisionType Collision::GetCollisionType() const { return m_collisionType; }
const CollisionComplexity &Collision::GetComplexity() const { return m_complexity; }
void Collision::InitializeBVH() { BuildBVHFromTriangles(); }
bool Collision::IntersectsBVH(const Collision &other) const { return Intersects(other); }
bool Collision::IsUsingBVH() const { return m_bvhRoot != nullptr; }
bool Collision::IsUsingOctree() const { return IsUsingBVH(); }
const CollisionTriangle &Collision::GetTriangle(size_t idx) const { return m_triangles[idx]; }
const std::vector<CollisionTriangle> &Collision::GetTriangles() const { return m_triangles; }
bool Collision::CheckCollisionWithBVH(const Collision& other, Vector3& outResponse) const {
    if (!other.IsUsingBVH() || !IntersectsAABB(other)) {
        return false;
    }

    // Get the center point of this collision
    Vector3 center = GetCenter();
    Vector3 size = GetSize();
    
    // Check points around the AABB
    const float checkDistance = size.y + 1.0f;
    bool hasCollision = false;
    float minY = FLT_MAX;

    // Check center and corners
    Vector3 checkPoints[] = {
        center,  // Center
        {center.x - size.x*0.5f, center.y, center.z}, // Left
        {center.x + size.x*0.5f, center.y, center.z}, // Right
        {center.x, center.y, center.z - size.z*0.5f}, // Front
        {center.x, center.y, center.z + size.z*0.5f}  // Back
    };

    for (const auto& point : checkPoints) {
        RayHit hit;
        Vector3 dir = {0, -1, 0};
        if (other.RaycastBVH(point, dir, checkDistance, hit)) {
            if (hit.hit && hit.distance < minY) {
                minY = hit.distance;
                outResponse = {0, hit.position.y - (center.y - size.y*0.5f), 0};
                hasCollision = true;
            }
        }
    }

    return hasCollision;
}

// ======================================================================================================================
// Optimized collision creation with caching

std::shared_ptr<Collision> Collision::CreateFromModelCached(void *model, const Matrix &transform)
{
    if (!model) return nullptr;

    // Create a hash from model pointer and transform matrix for caching
    size_t modelHash = std::hash<void*>{}(model);

    // Simple hash of transform matrix (can be improved)
    size_t transformHash = 0;
    const float* transformData = reinterpret_cast<const float*>(&transform);
    for (size_t i = 0; i < sizeof(Matrix) / sizeof(float); ++i)
    {
        transformHash = transformHash * 31 + std::hash<float>{}(transformData[i]);
    }

    size_t cacheKey = modelHash ^ (transformHash << 1);

    // Check if collision already exists in cache
    auto it = collisionCache.find(cacheKey);
    if (it != collisionCache.end())
    {
        auto cachedCollision = it->second.lock();
        if (cachedCollision)
        {
            return cachedCollision;
        }
        else
        {
            // Remove expired weak pointer
            collisionCache.erase(it);
        }
    }

    // Create new collision
    auto newCollision = std::make_shared<Collision>();
    newCollision->BuildFromModel(model, transform);

    // Cache the new collision
    collisionCache[cacheKey] = newCollision;

    return newCollision;
}

void Collision::ClearCollisionCache()
{
    collisionCache.clear();
}

// ==================== COLLISION POOL IMPLEMENTATION ====================

CollisionPool& CollisionPool::GetInstance()
{
    static CollisionPool instance;
    return instance;
}

CollisionPool::CollisionPool()
{
    TraceLog(LOG_INFO, "CollisionPool initialized");
}

CollisionPool::~CollisionPool()
{
    ClearPool();
    TraceLog(LOG_INFO, "CollisionPool destroyed");
}

std::shared_ptr<Collision> CollisionPool::AcquireCollision()
{
    std::lock_guard<std::mutex> lock(m_poolMutex);

    if (!m_collisionPool.empty())
    {
        auto collision = m_collisionPool.top();
        m_collisionPool.pop();
        m_activeCollisions.insert(collision);

        // Reset collision state
        collision->ResetCollisionState();

        TraceLog(LOG_DEBUG, "Acquired collision from pool. Pool size: %zu", m_collisionPool.size());
        return collision;
    }

    // Create new collision if pool is empty
    auto collision = std::make_shared<Collision>();
    m_activeCollisions.insert(collision);

    TraceLog(LOG_DEBUG, "Created new collision. Active collisions: %zu", m_activeCollisions.size());
    return collision;
}

void CollisionPool::ReleaseCollision(std::shared_ptr<Collision> collision)
{
    if (!collision) return;

    std::lock_guard<std::mutex> lock(m_poolMutex);

    // Remove from active set
    m_activeCollisions.erase(collision);

    // Clear collision data
    collision->ResetCollisionState();

    // Add to pool if under limit
    if (m_collisionPool.size() < m_maxPoolSize)
    {
        m_collisionPool.push(collision);
        TraceLog(LOG_DEBUG, "Released collision to pool. Pool size: %zu", m_collisionPool.size());
    }
    else
    {
        TraceLog(LOG_DEBUG, "Pool full, destroying collision");
    }
}

void CollisionPool::ClearPool()
{
    std::lock_guard<std::mutex> lock(m_poolMutex);

    while (!m_collisionPool.empty())
    {
        m_collisionPool.pop();
    }
    m_activeCollisions.clear();

    TraceLog(LOG_INFO, "Collision pool cleared");
}

void CollisionPool::CleanupUnusedCollisions()
{
    std::lock_guard<std::mutex> lock(m_poolMutex);

    // Remove expired weak pointers from cache
    auto& cache = Collision::GetCollisionCache();
    auto cacheIt = cache.begin();
    while (cacheIt != cache.end())
    {
        if (cacheIt->second.expired())
        {
            cacheIt = cache.erase(cacheIt);
        }
        else
        {
            ++cacheIt;
        }
    }

    TraceLog(LOG_DEBUG, "Cleaned up collision cache. Cache size: %zu", Collision::GetCollisionCacheSize());
}

void CollisionPool::CleanupExpiredCache()
{
    auto& cache = Collision::GetCollisionCache();
    size_t beforeSize = cache.size();
    auto it = cache.begin();

    while (it != cache.end())
    {
        if (it->second.expired())
        {
            it = cache.erase(it);
        }
        else
        {
            ++it;
        }
    }

    size_t afterSize = cache.size();
    if (beforeSize != afterSize)
    {
        TraceLog(LOG_INFO, "Cleaned up collision cache: %zu -> %zu entries", beforeSize, afterSize);
    }
}


