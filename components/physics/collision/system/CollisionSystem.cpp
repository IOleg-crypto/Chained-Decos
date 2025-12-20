#include "CollisionSystem.h"
#include <algorithm>
#include <cassert>
#include <cfloat>
#include <cmath>
#include <vector>


#include "../structures/collisionStructures.h"
#include <raylib.h>
#include <raymath.h>

Collision::Collision() { m_bounds = {{0, 0, 0}, {0, 0, 0}}; }
Collision::Collision(const Vector3 &center, const Vector3 &halfSize)
{
    m_bounds.min = Vector3Subtract(center, halfSize);
    m_bounds.max = Vector3Add(center, halfSize);
}

Collision::Collision(const Collision &other)
{
    m_bounds = other.m_bounds;
    m_collisionType = other.m_collisionType;
    m_triangles = other.m_triangles;
    m_isBuilt = other.m_isBuilt;

    if (other.m_bvhRoot)
    {
        BuildBVHFromTriangles();
    }
}

Collision &Collision::operator=(const Collision &other)
{
    if (this == &other)
        return *this;

    m_bounds = other.m_bounds;
    m_collisionType = other.m_collisionType;
    m_triangles = other.m_triangles;
    m_isBuilt = other.m_isBuilt;

    if (!m_triangles.empty() && (m_collisionType == CollisionType::BVH_ONLY ||
                                 m_collisionType == CollisionType::TRIANGLE_PRECISE))
    {
        BuildBVHFromTriangles();
    }

    return *this;
}

Collision::Collision(Collision &&other) noexcept
{
    m_bounds = other.m_bounds;
    m_collisionType = other.m_collisionType;
    m_triangles = std::move(other.m_triangles);
    m_bvhRoot = std::move(other.m_bvhRoot);
    m_isBuilt = other.m_isBuilt;
}
Collision &Collision::operator=(Collision &&other) noexcept
{
    if (this == &other)
        return *this;
    m_bounds = other.m_bounds;
    m_collisionType = other.m_collisionType;
    m_triangles = std::move(other.m_triangles);
    m_bvhRoot = std::move(other.m_bvhRoot);
    m_isBuilt = other.m_isBuilt;
    return *this;
}
Collision::~Collision() = default;

// ----------------- AABB -----------------
void Collision::Update(const Vector3 &center, const Vector3 &halfSize)
{
    m_bounds.min = Vector3Subtract(center, halfSize);
    m_bounds.max = Vector3Add(center, halfSize);
}

bool Collision::IntersectsAABB(const Collision &other) const
{
    // Use raylib's built-in AABB intersection
    return CheckCollisionBoxes(m_bounds, other.m_bounds);
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

    // Basic validation
    if (!rayModel || rayModel->meshCount <= 0)
    {
        TraceLog(LOG_WARNING, "Collision::BuildFromModel() - Invalid model or no meshes");
        return;
    }

    size_t totalTriangles = 0;
    for (int meshIdx = 0; meshIdx < rayModel->meshCount; ++meshIdx)
    {
        Mesh &mesh = rayModel->meshes[meshIdx];
        if (mesh.vertices && mesh.indices && mesh.triangleCount > 0)
        {
            totalTriangles += mesh.triangleCount;
        }
    }

    if (totalTriangles == 0)
    {
        TraceLog(LOG_WARNING, "Collision::BuildFromModel() - No triangles found in model");
        return;
    }

    // Pre-allocate memory
    m_triangles.reserve(totalTriangles);

    // Process all meshes
    for (int meshIdx = 0; meshIdx < rayModel->meshCount; ++meshIdx)
    {
        Mesh &mesh = rayModel->meshes[meshIdx];

        if (!mesh.vertices || !mesh.indices || mesh.triangleCount == 0)
            continue;

        // Batch process triangles for this mesh
        const int triangleCount = mesh.triangleCount;
        const unsigned short *indices = mesh.indices;

        for (int i = 0; i < triangleCount; ++i)
        {
            const int idx = i * 3;

            const int i0 = indices[idx + 0];
            const int i1 = indices[idx + 1];
            const int i2 = indices[idx + 2];

            if (i0 >= mesh.vertexCount || i1 >= mesh.vertexCount || i2 >= mesh.vertexCount)
                continue;

            const float *v0Ptr = &mesh.vertices[i0 * 3];
            const float *v1Ptr = &mesh.vertices[i1 * 3];
            const float *v2Ptr = &mesh.vertices[i2 * 3];

            Vector3 v0 = {v0Ptr[0], v0Ptr[1], v0Ptr[2]};
            Vector3 v1 = {v1Ptr[0], v1Ptr[1], v1Ptr[2]};
            Vector3 v2 = {v2Ptr[0], v2Ptr[1], v2Ptr[2]};

            // Basic validation
            if (!std::isfinite(v0.x) || !std::isfinite(v0.y) || !std::isfinite(v0.z) ||
                !std::isfinite(v1.x) || !std::isfinite(v1.y) || !std::isfinite(v1.z) ||
                !std::isfinite(v2.x) || !std::isfinite(v2.y) || !std::isfinite(v2.z))
                continue;

            // Check for degenerate triangles
            Vector3 edge1 = Vector3Subtract(v1, v0);
            Vector3 edge2 = Vector3Subtract(v2, v0);
            if (Vector3LengthSqr(Vector3CrossProduct(edge1, edge2)) < 1e-12f)
                continue;

            // Transform vertices to world coordinates (apply once)
            v0 = Vector3Transform(v0, transform);
            v1 = Vector3Transform(v1, transform);
            v2 = Vector3Transform(v2, transform);

            // Add triangle
            m_triangles.emplace_back(v0, v1, v2);
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
        Vector3 corners[8] = {{bb.min.x, bb.min.y, bb.min.z}, {bb.max.x, bb.min.y, bb.min.z},
                              {bb.min.x, bb.max.y, bb.min.z}, {bb.min.x, bb.min.y, bb.max.z},
                              {bb.max.x, bb.max.y, bb.min.z}, {bb.min.x, bb.max.y, bb.max.z},
                              {bb.max.x, bb.min.y, bb.max.z}, {bb.max.x, bb.max.y, bb.max.z}};

        Vector3 tmin = {FLT_MAX, FLT_MAX, FLT_MAX};
        Vector3 tmax = {-FLT_MAX, -FLT_MAX, -FLT_MAX};

        for (const auto &corner : corners)
        {
            Vector3 tc = Vector3Transform(corner, transform);
            tmin.x = fminf(tmin.x, tc.x);
            tmin.y = fminf(tmin.y, tc.y);
            tmin.z = fminf(tmin.z, tc.z);
            tmax.x = fmaxf(tmax.x, tc.x);
            tmax.y = fmaxf(tmax.y, tc.y);
            tmax.z = fmaxf(tmax.z, tc.z);
        }

        m_bounds.min = tmin;
        m_bounds.max = tmax;
    }

    m_isBuilt = true;
}

void Collision::BuildFromModelWithType(void *model, CollisionType type, const Matrix &transform)
{
    m_collisionType = type;
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

void Collision::UpdateAABBFromTriangles()
{
    if (m_triangles.empty())
        return;

    // Initialize with first triangle vertices
    const CollisionTriangle &firstTri = m_triangles[0];
    Vector3 minP = firstTri.V0();
    Vector3 maxP = firstTri.V0();

    // Unroll first few iterations for better performance
    ExpandAABB(minP, maxP, firstTri.V1());
    ExpandAABB(minP, maxP, firstTri.V2());

    // Process remaining triangles in optimized loop
    const size_t triangleCount = m_triangles.size();
    for (size_t i = 1; i < triangleCount; ++i)
    {
        const CollisionTriangle &t = m_triangles[i];
        // Process all three vertices at once for better cache performance
        ExpandAABB(minP, maxP, t.V0());
        ExpandAABB(minP, maxP, t.V1());
        ExpandAABB(minP, maxP, t.V2());
    }

    m_bounds.min = minP;
    m_bounds.max = maxP;
}

void Collision::AddTriangle(const CollisionTriangle &triangle) { m_triangles.push_back(triangle); }

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
                  if (!std::isfinite(ca_val))
                      ca_val = 0.0f;
                  if (!std::isfinite(cb_val))
                      cb_val = 0.0f;

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
    TraceLog(LOG_DEBUG,
             "Collision::BuildBVHFromTriangles() - Starting BVH build for collision object");

    if (m_triangles.empty())
    {
        TraceLog(LOG_DEBUG, "Collision::BuildBVHFromTriangles() - No triangles to build BVH");
        m_bvhRoot.reset();
        return;
    }

    // Validate triangles before building BVH
    size_t validTriangles = 0;
    for (const auto &tri : m_triangles)
    {
        if (std::isfinite(tri.V0().x) && std::isfinite(tri.V0().y) && std::isfinite(tri.V0().z) &&
            std::isfinite(tri.V1().x) && std::isfinite(tri.V1().y) && std::isfinite(tri.V1().z) &&
            std::isfinite(tri.V2().x) && std::isfinite(tri.V2().y) && std::isfinite(tri.V2().z))
        {
            validTriangles++;
        }
    }

    TraceLog(LOG_DEBUG,
             "Collision::BuildBVHFromTriangles() - Found %zu valid triangles out of %zu total",
             validTriangles, m_triangles.size());

    if (validTriangles == 0)
    {
        TraceLog(LOG_ERROR, "Collision::BuildBVHFromTriangles() - No valid triangles found");
        m_bvhRoot.reset();
        return;
    }

    if (validTriangles < m_triangles.size())
    {
        TraceLog(
            LOG_WARNING,
            "Collision::BuildBVHFromTriangles() - Found %zu invalid triangles out of %zu total",
            m_triangles.size() - validTriangles, m_triangles.size());
    }

    // Build BVH directly with existing triangles - no copying needed
    try
    {
        m_bvhRoot = BuildBVHNode(m_triangles, 0);
        TraceLog(LOG_INFO,
                 "Collision::BuildBVHFromTriangles() - Successfully built BVH with %zu triangles",
                 validTriangles);
    }
    catch (const std::exception &e)
    {
        TraceLog(LOG_ERROR, "Collision::BuildBVHFromTriangles() - Failed to build BVH: %s",
                 e.what());
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

// ----------------- BVH raycast traversal -----------------
bool Collision::RaycastBVHNode(const BVHNode *node, const Vector3 &origin, const Vector3 &dir,
                               float maxDistance, RayHit &outHit) const
{
    if (!node)
        return false;
    // Use raylib for AABB-ray intersection test
    Ray ray = {origin, dir};
    BoundingBox box = {node->min, node->max};
    RayCollision collision = GetRayCollisionBox(ray, box);
    if (!collision.hit || collision.distance > maxDistance)
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

// ----------------- Intersects (AABB broad, BVH narrow) -----------------
// Helper: check if other's BVH has any leaf AABB overlapping this Collision's AABB
// Optimized Triangle-AABB SAT test with early exit optimizations
static bool TriangleAABBOverlapSAT(const CollisionTriangle &tri, const Vector3 &bmin,
                                   const Vector3 &bmax)
{
    // Quick AABB-AABB test first (much faster than full SAT)
    Vector3 triMin = tri.V0();
    Vector3 triMax = tri.V0();

    // Calculate triangle AABB inline for speed
    if (tri.V1().x < triMin.x)
        triMin.x = tri.V1().x;
    else if (tri.V1().x > triMax.x)
        triMax.x = tri.V1().x;
    if (tri.V1().y < triMin.y)
        triMin.y = tri.V1().y;
    else if (tri.V1().y > triMax.y)
        triMax.y = tri.V1().y;
    if (tri.V1().z < triMin.z)
        triMin.z = tri.V1().z;
    else if (tri.V1().z > triMax.z)
        triMax.z = tri.V1().z;

    if (tri.V2().x < triMin.x)
        triMin.x = tri.V2().x;
    else if (tri.V2().x > triMax.x)
        triMax.x = tri.V2().x;
    if (tri.V2().y < triMin.y)
        triMin.y = tri.V2().y;
    else if (tri.V2().y > triMax.y)
        triMax.y = tri.V2().y;
    if (tri.V2().z < triMin.z)
        triMin.z = tri.V2().z;
    else if (tri.V2().z > triMax.z)
        triMax.z = tri.V2().z;

    // Early exit: if triangle AABB doesn't overlap box AABB, no intersection
    if (triMax.x < bmin.x || triMin.x > bmax.x)
        return false;
    if (triMax.y < bmin.y || triMin.y > bmax.y)
        return false;
    if (triMax.z < bmin.z || triMin.z > bmax.z)
        return false;

    // If we get here, AABBs overlap, so we need full SAT test
    // Centered box with half extents
    Vector3 c = {(bmin.x + bmax.x) * 0.5f, (bmin.y + bmax.y) * 0.5f, (bmin.z + bmax.z) * 0.5f};
    Vector3 h = {(bmax.x - bmin.x) * 0.5f, (bmax.y - bmin.y) * 0.5f, (bmax.z - bmin.z) * 0.5f};

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
    if (!axisTest(ax))
        return false;
    if (!axisTest(ay))
        return false;
    if (!axisTest(az))
        return false;

    // 2) Test triangle plane normal
    Vector3 e0 = Vector3Subtract(v1, v0);
    Vector3 e1 = Vector3Subtract(v2, v0);
    Vector3 n = Vector3CrossProduct(e0, e1);
    if (!axisTest(n))
        return false;

    // 3) Test cross products of triangle edges with box axes (9 axes)
    Vector3 e2 = Vector3Subtract(v2, v1);
    const Vector3 edges[3] = {e0, e1, e2};
    const Vector3 boxAxes[3] = {ax, ay, az};
    for (const Vector3 &e : edges)
    {
        for (const Vector3 &ba : boxAxes)
        {
            Vector3 a = Vector3CrossProduct(e, ba);
            if (!axisTest(a))
                return false;
        }
    }

    return true; // no separating axis found
}

static bool BVHOverlapsAABB(const BVHNode *node, const Collision &aabbCollider)
{
    if (!node)
        return false;
    // Quick reject: node AABB vs collider AABB
    Collision nodeBox{Vector3Scale(Vector3Add(node->min, node->max), 0.5f),
                      Vector3Scale(Vector3Subtract(node->max, node->min), 0.5f)};
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
        if (!otherIntoThis)
            return false;
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
Vector3 Collision::GetSize() const { return Vector3Subtract(m_bounds.max, m_bounds.min); }
Vector3 Collision::GetCenter() const
{
    return Vector3Scale(Vector3Add(m_bounds.min, m_bounds.max), 0.5f);
}
size_t Collision::GetTriangleCount() const { return m_triangles.size(); }
bool Collision::HasTriangleData() const { return !m_triangles.empty(); }

CollisionType Collision::GetCollisionType() const { return m_collisionType; }
void Collision::InitializeBVH() { BuildBVHFromTriangles(); }
const CollisionTriangle &Collision::GetTriangle(size_t idx) const { return m_triangles[idx]; }
const std::vector<CollisionTriangle> &Collision::GetTriangles() const { return m_triangles; }

// ======================================================================================================================






