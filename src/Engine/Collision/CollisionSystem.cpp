#include "CollisionSystem.h"
#include <cassert>
#include <cmath>

#include "CollisionStructures.h"

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
    m_triangles = other.m_triangles; // копіюємо тріанґли
    m_bvhRoot = nullptr;             // BVH можна побудувати заново
    m_isBuilt = other.m_isBuilt;
    m_stats = other.m_stats;
}

Collision& Collision::operator=(const Collision& other)
{
    if (this == &other) return *this;
    m_min = other.m_min;
    m_max = other.m_max;
    m_collisionType = other.m_collisionType;
    m_complexity = other.m_complexity;
    m_triangles = other.m_triangles;
    m_bvhRoot = nullptr;
    m_isBuilt = other.m_isBuilt;
    m_stats = other.m_stats;
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

// ----------------- Build from model (stub) -----------------
void Collision::BuildFromModel(void *model, const Matrix &transform)
{
    UpdateAABBFromTriangles();
    BuildBVHFromTriangles();
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
    Vector3 minP = m_triangles[0].V0();
    Vector3 maxP = minP;
    for (const auto &t : m_triangles)
    {
        ExpandAABB(minP, maxP, t.V0());
        ExpandAABB(minP, maxP, t.V1());
        ExpandAABB(minP, maxP, t.V2());
    }
    m_min = minP;
    m_max = maxP;
}

// ----------------- BVH build -----------------
static int const MAX_TRIANGLES_PER_LEAF = 8;
static int const MAX_BVH_DEPTH = 48;

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
        node->triangles = tris; // leaf
        return node;
    }

    // choose split axis by longest axis
    Vector3 ext = Vector3Subtract(maxB, minB);
    int axis = 0;
    if (ext.y > ext.x && ext.y >= ext.z)
        axis = 1;
    else if (ext.z > ext.x && ext.z > ext.y)
        axis = 2;

    // sort by centroid along axis
    std::ranges::sort(tris,
                      [axis](const CollisionTriangle &a, const CollisionTriangle &b)
                      {
                          float ca = (a.V0().x + a.V1().x + a.V2().x) / 3.0f;
                          float cb = (b.V0().x + b.V1().x + b.V2().x) / 3.0f;
                          if (axis == 1)
                          {
                              ca = (a.V0().y + a.V1().y + a.V2().y) / 3.0f;
                              cb = (b.V0().y + b.V1().y + b.V2().y) / 3.0f;
                          }
                          if (axis == 2)
                          {
                              ca = (a.V0().z + a.V1().z + a.V2().z) / 3.0f;
                              cb = (b.V0().z + b.V1().z + b.V2().z) / 3.0f;
                          }
                          return ca < cb;
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
        m_bvhRoot.reset();
        return;
    }
    // copy triangles so build can reorder
    std::vector<CollisionTriangle> copy = m_triangles;
    m_bvhRoot = BuildBVHNode(copy, 0);
}

// ----------------- Ray/triangle (Möller–Trumbore) -----------------
bool Collision::RayIntersectsTriangle(const Vector3 &orig, const Vector3 &dir,
                                      const CollisionTriangle &tri, RayHit &outHit)
{
    const float EPS = 1e-6f;
    Vector3 edge1 = Vector3Subtract(tri.V1(), tri.V0());
    Vector3 edge2 = Vector3Subtract(tri.V2(), tri.V0());
    Vector3 h = Vector3CrossProduct(dir, edge2);
    float a = Vector3DotProduct(edge1, h);
    if (a > -EPSILON && a < EPSILON)
        return false; // parallel
    float f = 1.0f / a;
    Vector3 s = Vector3Subtract(orig, tri.V0());
    float u = f * Vector3DotProduct(s, h);
    if (u < 0.0f || u > 1.0f)
        return false;
    Vector3 q = Vector3CrossProduct(s, edge1);
    float v = f * Vector3DotProduct(dir, q);
    if (v < 0.0f || u + v > 1.0f)
        return false;
    float t = f * Vector3DotProduct(edge2, q);
    if (t > EPS)
    {
        outHit.hit = true;
        outHit.distance = t;
        outHit.position = Vector3Add(orig, Vector3Scale(dir, t));
        outHit.normal = Vector3Normalize(Vector3CrossProduct(edge1, edge2));
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
        float invD = 1.0f / ((i == 0) ? dir.x : (i == 1 ? dir.y : dir.z));
        float o = (i == 0) ? origin.x : (i == 1 ? origin.y : origin.z);
        float mn = (i == 0) ? min.x : (i == 1 ? min.y : min.z);
        float mx = (i == 0) ? max.x : (i == 1 ? max.y : max.z);
        float t0 = (mn - o) * invD;
        float t1 = (mx - o) * invD;
        if (invD < 0.0f)
            std::swap(t0, t1);
        tmin = std::max(tmin, t0);
        tmax = std::min(tmax, t1);
        if (tmax <= tmin)
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
        for (const auto &tri : node->triangles)
        {
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
    bool ok = RaycastBVHNode(m_bvhRoot.get(), origin, dir, maxDistance, outHit);
    return ok;
}

// ----------------- Point-in-mesh via raycast (count intersections) -----------------
bool Collision::ContainsPointBVH(const Vector3 &point) const
{
    // Cast ray in +X direction and count intersections
    Vector3 dir = {1.0f, 0.0001f, 0.0002f}; // slight offset to avoid coplanar degeneracy
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
bool Collision::Intersects(const Collision &other) const
{
    // broad-phase
    if (!IntersectsAABB(other))
        return false;

    // narrow-phase: if both have BVH, check triangle-triangle intersections (expensive).
    // For performance, we can test each leaf tri AABB against other's BVH.
    if (m_bvhRoot && other.m_bvhRoot)
    {
        // naive: traverse my leaves and test triangles vs other's BVH AABB
        std::vector<const BVHNode *> stack;
        stack.push_back(m_bvhRoot.get());
        while (!stack.empty())
        {
            const BVHNode *n = stack.back();
            stack.pop_back();
            if (!IntersectsAABB(Collision{Vector3Scale(Vector3Add(n->min, n->max), 0.5f),
                                          Vector3Scale(Vector3Subtract(n->max, n->min), 0.5f)}))
            {
                // this AABB vs THIS object's AABB - but we need to test against other: quick AABB
                // vs other root
            }
            // For brevity: fallback to coarse check: return true (since AABB overlapped).
            // In production: implement detailed triangle-vs-BVH overlap test here.
            return true;
        }
    }

    // if no BVH or detailed check not implemented, fall back to AABB result
    return true;
}

// ======================================================================================================================
void Collision::SetCollisionType(CollisionType type) { m_collisionType = type; }
bool BVHNode::IsLeaf() const { return !left && !right; }
Vector3 Collision::GetSize() const { return Vector3Subtract(m_max, m_min); }
Vector3 Collision::GetCenter() const { return Vector3Scale(Vector3Add(m_min, m_max), 0.5f); }
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
