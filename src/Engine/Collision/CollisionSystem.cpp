#include "CollisionSystem.h"
#include <cassert>
#include <cmath>
#include <cfloat>

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
    m_triangles = other.m_triangles;
    m_isBuilt = other.m_isBuilt;
    m_stats = other.m_stats;

    // Копіюємо BVH тільки якщо він використовується
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

    // Якщо є трикутники і тип BVH - перебудовуємо BVH
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

// ----------------- Build from model (stub) -----------------
void Collision::BuildFromModel(void *model, const Matrix &transform)
{
    // Додаємо трикутники з трансформацією
    Model *rayModel = static_cast<Model *>(model);
    if (rayModel)
    {
        for (int meshIdx = 0; meshIdx < rayModel->meshCount; ++meshIdx)
        {
            Mesh &mesh = rayModel->meshes[meshIdx];
            if (!mesh.vertices || !mesh.indices) continue;
            for (int i = 0; i < mesh.triangleCount; ++i)
            {
                int i0 = mesh.indices[i * 3 + 0];
                int i1 = mesh.indices[i * 3 + 1];
                int i2 = mesh.indices[i * 3 + 2];
                Vector3 v0 = { mesh.vertices[i0 * 3 + 0], mesh.vertices[i0 * 3 + 1], mesh.vertices[i0 * 3 + 2] };
                Vector3 v1 = { mesh.vertices[i1 * 3 + 0], mesh.vertices[i1 * 3 + 1], mesh.vertices[i1 * 3 + 2] };
                Vector3 v2 = { mesh.vertices[i2 * 3 + 0], mesh.vertices[i2 * 3 + 1], mesh.vertices[i2 * 3 + 2] };
                // Трансформуємо у світові координати
                v0 = Vector3Transform(v0, transform);
                v1 = Vector3Transform(v1, transform);
                v2 = Vector3Transform(v2, transform);
                m_triangles.emplace_back(v0, v1, v2);
            }
        }
        TraceLog(LOG_INFO, "Collision triangles: %zu", m_triangles.size());
    }

    // Try to build BVH from triangles if available
    UpdateAABBFromTriangles();
    BuildBVHFromTriangles();

    // If we have no triangles/BVH, fall back to model's bounding box
    if (!m_bvhRoot)
    {
        Model *rayModel = static_cast<Model *>(model);
        if (rayModel)
        {
            BoundingBox bb = GetModelBoundingBox(*rayModel);
            // Compute 8 corners of the original AABB
            Vector3 corners[8] = {
                {bb.min.x, bb.min.y, bb.min.z}, {bb.max.x, bb.min.y, bb.min.z},
                {bb.min.x, bb.max.y, bb.min.z}, {bb.min.x, bb.min.y, bb.max.z},
                {bb.max.x, bb.max.y, bb.min.z}, {bb.min.x, bb.max.y, bb.max.z},
                {bb.max.x, bb.min.y, bb.max.z}, {bb.max.x, bb.max.y, bb.max.z}};

            // Transform corners
            Vector3 tmin = {FLT_MAX, FLT_MAX, FLT_MAX};
            Vector3 tmax = {-FLT_MAX, -FLT_MAX, -FLT_MAX};
            for (auto &c : corners)
            {
                Vector3 tc = Vector3Transform(c, transform);
                tmin.x = fminf(tmin.x, tc.x); tmin.y = fminf(tmin.y, tc.y); tmin.z = fminf(tmin.z, tc.z);
                tmax.x = fmaxf(tmax.x, tc.x); tmax.y = fmaxf(tmax.y, tc.y); tmax.z = fmaxf(tmax.z, tc.z);
            }

            m_min = tmin;
            m_max = tmax;
        }
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
// Helper: check if other's BVH has any leaf AABB overlapping this Collision's AABB
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
        // Leaf contains triangles bounded by node AABB; treat as overlap
        return !node->triangles.empty();
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
