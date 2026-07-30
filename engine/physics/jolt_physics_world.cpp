#include "jolt_physics_world.h"
#include "engine/core/log.h"

#include <Jolt/Core/Factory.h>
#include <Jolt/Geometry/Triangle.h>
#include <Jolt/Physics/Body/MassProperties.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/ConvexHullShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <Jolt/Physics/Collision/Shape/ScaledShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/RegisterTypes.h>

#include <chrono>
#include <cmath>
#include <cstdint>
#include <future>
#include <mutex>

namespace Chained
{

// ─────────────────────────────────────────────────────────────────────────────
// Layer setup — two layers: NON_MOVING (static) and MOVING (dynamic/kinematic)
// ─────────────────────────────────────────────────────────────────────────────
namespace Layers
{
static constexpr JPH::ObjectLayer NON_MOVING = 0;
static constexpr JPH::ObjectLayer MOVING = 1;
static constexpr JPH::ObjectLayer NUM_LAYERS = 2;
} // namespace Layers

class ObjectLayerPairFilterImpl : public JPH::ObjectLayerPairFilter
{
public:
    bool ShouldCollide(JPH::ObjectLayer a, JPH::ObjectLayer b) const override
    {
        switch (a)
        {
        case Layers::NON_MOVING:
            return b == Layers::MOVING;
        case Layers::MOVING:
            return true;
        default:
            return false;
        }
    }
};

namespace BroadPhaseLayers
{
static constexpr JPH::BroadPhaseLayer NON_MOVING(0);
static constexpr JPH::BroadPhaseLayer MOVING(1);
static constexpr uint32_t NUM_LAYERS = 2;
} // namespace BroadPhaseLayers

class BPLayerInterfaceImpl final : public JPH::BroadPhaseLayerInterface
{
public:
    BPLayerInterfaceImpl()
    {
        m_Map[Layers::NON_MOVING] = BroadPhaseLayers::NON_MOVING;
        m_Map[Layers::MOVING] = BroadPhaseLayers::MOVING;
    }
    uint32_t GetNumBroadPhaseLayers() const override
    {
        return BroadPhaseLayers::NUM_LAYERS;
    }
    JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer layer) const override
    {
        return m_Map[layer];
    }
#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
    const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer layer) const override
    {
        return (JPH::BroadPhaseLayer::Type)layer == 0 ? "NON_MOVING" : "MOVING";
    }
#endif
private:
    JPH::BroadPhaseLayer m_Map[Layers::NUM_LAYERS];
};

class ObjectVsBroadPhaseLayerFilterImpl : public JPH::ObjectVsBroadPhaseLayerFilter
{
public:
    bool ShouldCollide(JPH::ObjectLayer layer, JPH::BroadPhaseLayer bpLayer) const override
    {
        switch (layer)
        {
        case Layers::NON_MOVING:
            return bpLayer == BroadPhaseLayers::MOVING;
        case Layers::MOVING:
            return true;
        default:
            return false;
        }
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// File-scope singletons — survive JoltPhysicsWorld::ResetWorld() rebuilds
// ─────────────────────────────────────────────────────────────────────────────
static BPLayerInterfaceImpl s_BPLayerInterface;
static ObjectVsBroadPhaseLayerFilterImpl s_ObjVsBPFilter;
static ObjectLayerPairFilterImpl s_ObjVsObjFilter;

// ─────────────────────────────────────────────────────────────────────────────
// JoltPhysicsWorld
// ─────────────────────────────────────────────────────────────────────────────
JoltPhysicsWorld::JoltPhysicsWorld()
{
    m_TempAllocator = std::make_unique<JPH::TempAllocatorImpl>(32 * 1024 * 1024);
    unsigned int hwThreads = std::thread::hardware_concurrency();
    int numThreads = std::max(1, (int)hwThreads - 1);
    m_JobSystem =
        std::make_unique<JPH::JobSystemThreadPool>(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, numThreads);

    m_PhysicsSystem.Init(65536, 0, 65536, 65536, s_BPLayerInterface, s_ObjVsBPFilter, s_ObjVsObjFilter);

    m_PhysicsSystem.SetGravity(JPH::Vec3(0.0f, -9.81f, 0.0f));

    m_ContactListener.SetGroundedTracker(&m_GroundedBodies, &m_GroundedMutex);
    m_PhysicsSystem.SetContactListener(&m_ContactListener);

    CH_CORE_INFO("Jolt Physics World Initialized with ContactListener.");
}

JoltPhysicsWorld::~JoltPhysicsWorld() = default;

void JoltPhysicsWorld::ClearShapeCache()
{
    std::lock_guard<std::mutex> lock(m_CacheMutex);
    m_MeshShapeCache.clear();
    m_ConvexHullCache.clear();
}

JPH::ShapeRefC JoltPhysicsWorld::BuildShape(const PhysicsBodyDesc& desc)
{
    JPH::ShapeRefC shape;

    switch (desc.Shape)
    {
    case ColliderType::Box: {
        JPH::BoxShapeSettings boxSettings(JPH::Vec3(desc.Dimensions.x, desc.Dimensions.y, desc.Dimensions.z));
        shape = boxSettings.Create().Get();
        if (!shape)
        {
            CH_CORE_WARN("Physics: shape creation failed — falling back to unit box.");
            shape = JPH::BoxShapeSettings(JPH::Vec3(0.5f, 0.5f, 0.5f)).Create().Get();
            if (!shape)
            {
                CH_CORE_ERROR("Physics: unit-box fallback shape creation failed.");
            }
        }
        break;
    }
    case ColliderType::Sphere: {
        JPH::SphereShapeSettings sphereSettings(desc.Dimensions.x);
        shape = sphereSettings.Create().Get();
        if (!shape)
        {
            CH_CORE_WARN("Physics: shape creation failed — falling back to unit box.");
            shape = JPH::BoxShapeSettings(JPH::Vec3(0.5f, 0.5f, 0.5f)).Create().Get();
            if (!shape)
            {
                CH_CORE_ERROR("Physics: unit-box fallback shape creation failed.");
            }
        }
        break;
    }
    case ColliderType::Capsule: {
        JPH::CapsuleShapeSettings capsuleSettings(desc.Dimensions.y, desc.Dimensions.x);
        shape = capsuleSettings.Create().Get();
        if (!shape)
        {
            CH_CORE_WARN("Physics: shape creation failed — falling back to unit box.");
            shape = JPH::BoxShapeSettings(JPH::Vec3(0.5f, 0.5f, 0.5f)).Create().Get();
            if (!shape)
            {
                CH_CORE_ERROR("Physics: unit-box fallback shape creation failed.");
            }
        }
        break;
    }
    case ColliderType::Mesh: {
        if (desc.Triangles.empty())
        {
            CH_CORE_WARN("Physics: MeshShape requested but no triangles provided — falling back to unit box.");
            shape = JPH::BoxShapeSettings(JPH::Vec3(0.5f, 0.5f, 0.5f)).Create().Get();
            if (!shape)
            {
                CH_CORE_ERROR("Physics: unit-box fallback shape creation failed.");
            }
            break;
        }

        if (!desc.IsStatic)
        {
            // Convex hull path for dynamic meshes — check cache first
            std::string convexKey = desc.CacheKey.empty() ? "" : desc.CacheKey + "_convex";
            if (!convexKey.empty())
            {
                std::lock_guard<std::mutex> lock(m_CacheMutex);
                auto it = m_ConvexHullCache.find(convexKey);
                if (it != m_ConvexHullCache.end())
                {
                    shape = it->second;
                    if (desc.MeshScale != glm::vec3(1.0f))
                    {
                        JPH::ScaledShapeSettings scaledSettings(
                            shape, JPH::Vec3(desc.MeshScale.x, desc.MeshScale.y, desc.MeshScale.z));
                        auto scaledResult = scaledSettings.Create();
                        if (!scaledResult.HasError())
                        {
                            shape = scaledResult.Get();
                        }
                    }
                    CH_CORE_TRACE("Physics: Reused cached convex hull [key='{}']", convexKey);
                    break;
                }
            }

            constexpr float kGridCell = 0.05f;
            std::unordered_set<uint64_t> seen;
            JPH::Array<JPH::Vec3> points;

            auto tryAdd = [&](const glm::vec3& p) {
                auto quant = [](float v) { return (int64_t)std::floor(v / kGridCell); };
                uint64_t key = (uint64_t)(quant(p.x) & 0x1FFFFF) | ((uint64_t)(quant(p.y) & 0x1FFFFF) << 21) |
                               ((uint64_t)(quant(p.z) & 0x1FFFFF) << 42);
                if (seen.insert(key).second)
                {
                    points.push_back(JPH::Vec3(p.x, p.y, p.z));
                }
            };

            for (const auto& t : desc.Triangles)
            {
                tryAdd(t.V0);
                tryAdd(t.V1);
                tryAdd(t.V2);
            }

            if (points.size() > 4096)
            {
                CH_CORE_WARN("Physics: Dynamic mesh has {} unique points after dedup ({} raw tris) — this looks "
                             "like static level geometry marked non-static. A convex hull will be a crude blob; "
                             "consider making this body Static or Kinematic instead.",
                             points.size(), desc.Triangles.size());
            }

            // Cap convex hull at 256 points — Jolt's hull builder struggles above this.
            // Downsample uniformly if needed.
            constexpr size_t kMaxConvexPoints = 256;
            if (points.size() > kMaxConvexPoints)
            {
                CH_CORE_WARN("Physics: Convex hull downsampled from {} to {} points.", points.size(), kMaxConvexPoints);
                JPH::Array<JPH::Vec3> downsampled;
                downsampled.reserve(kMaxConvexPoints);
                size_t step = points.size() / kMaxConvexPoints;
                for (size_t i = 0; i < points.size() && downsampled.size() < kMaxConvexPoints; i += step)
                {
                    downsampled.push_back(points[i]);
                }
                points = std::move(downsampled);
            }

            JPH::ConvexHullShapeSettings hullSettings(points);
            auto hullResult = hullSettings.Create();
            if (hullResult.HasError())
            {
                CH_CORE_ERROR("Physics: ConvexHull build for dynamic mesh failed: {} — falling back to unit box.",
                              hullResult.GetError().c_str());
                shape = JPH::BoxShapeSettings(JPH::Vec3(0.5f, 0.5f, 0.5f)).Create().Get();
                if (!shape)
                {
                    CH_CORE_ERROR("Physics: unit-box fallback shape creation failed.");
                }
                break;
            }

            JPH::ShapeRefC hull = hullResult.Get();
            if (!convexKey.empty())
            {
                std::lock_guard<std::mutex> lock(m_CacheMutex);
                m_ConvexHullCache[convexKey] = hull;
            }

            shape = hull;
            if (desc.MeshScale != glm::vec3(1.0f))
            {
                JPH::ScaledShapeSettings scaledSettings(
                    hull, JPH::Vec3(desc.MeshScale.x, desc.MeshScale.y, desc.MeshScale.z));
                auto scaledResult = scaledSettings.Create();
                if (!scaledResult.HasError())
                {
                    shape = scaledResult.Get();
                }
                else
                {
                    CH_CORE_WARN("Physics: ScaledShape build failed: {} — using unscaled hull.",
                                 scaledResult.GetError().c_str());
                }
            }
            CH_CORE_INFO("Physics: Dynamic mesh body approximated with ConvexHull ({} points, {} raw tris).",
                         points.size(), desc.Triangles.size());
            break;
        }

        // Static mesh: BVH cache keyed by model path
        JPH::ShapeRefC baseShape;
        bool cached = false;
        if (!desc.CacheKey.empty())
        {
            std::lock_guard<std::mutex> lock(m_CacheMutex);
            auto it = m_MeshShapeCache.find(desc.CacheKey);
            if (it != m_MeshShapeCache.end())
            {
                baseShape = it->second;
                cached = true;
                CH_CORE_TRACE("Physics: Reused cached mesh BVH [key='{}']", desc.CacheKey);
            }
        }

        if (!cached)
        {
            const auto buildStart = std::chrono::steady_clock::now();
            JPH::TriangleList joltTris;
            joltTris.reserve(desc.Triangles.size());
            for (const auto& t : desc.Triangles)
            {
                JPH::Triangle tri(JPH::Float3(t.V0.x, t.V0.y, t.V0.z), JPH::Float3(t.V1.x, t.V1.y, t.V1.z),
                                  JPH::Float3(t.V2.x, t.V2.y, t.V2.z));
                JPH::Vec3 v0 = JPH::Vec3::sLoadFloat3Unsafe(tri.mV[0]);
                JPH::Vec3 v1 = JPH::Vec3::sLoadFloat3Unsafe(tri.mV[1]);
                JPH::Vec3 v2 = JPH::Vec3::sLoadFloat3Unsafe(tri.mV[2]);
                JPH::Vec3 e1 = v1 - v0;
                JPH::Vec3 e2 = v2 - v0;
                if (e1.Cross(e2).LengthSq() < 1e-20f)
                {
                    continue;
                }
                joltTris.push_back(tri);
            }

            if (joltTris.empty())
            {
                CH_CORE_WARN("Physics: All mesh triangles degenerate — falling back to unit box.");
                shape = JPH::BoxShapeSettings(JPH::Vec3(0.5f, 0.5f, 0.5f)).Create().Get();
                if (!shape)
                {
                    CH_CORE_ERROR("Physics: unit-box fallback shape creation failed.");
                }
                break;
            }

            JPH::MeshShapeSettings s(std::move(joltTris));
            s.mBuildQuality = desc.UseFastBuildQuality ? JPH::MeshShapeSettings::EBuildQuality::FavorBuildSpeed
                                                       : JPH::MeshShapeSettings::EBuildQuality::FavorRuntimePerformance;
            auto result = s.Create();
            if (result.HasError())
            {
                CH_CORE_ERROR("Physics: MeshShape build failed: {} — falling back to unit box.",
                              result.GetError().c_str());
                shape = JPH::BoxShapeSettings(JPH::Vec3(0.5f, 0.5f, 0.5f)).Create().Get();
                if (!shape)
                {
                    CH_CORE_ERROR("Physics: unit-box fallback shape creation failed.");
                }
                break;
            }

            baseShape = result.Get();
            if (!desc.CacheKey.empty())
            {
                std::lock_guard<std::mutex> lock(m_CacheMutex);
                m_MeshShapeCache[desc.CacheKey] = baseShape;
            }

            const auto buildEnd = std::chrono::steady_clock::now();
            const double buildMs = std::chrono::duration<double, std::milli>(buildEnd - buildStart).count();
            CH_CORE_INFO("Physics: Built mesh BVH ({} tris) in {:.2f} ms{} [key='{}']", joltTris.size(), buildMs,
                         desc.CacheKey.empty() ? " (uncached)" : " (freshly built, cached for reuse)", desc.CacheKey);
        }

        shape = baseShape;
        if (desc.MeshScale != glm::vec3(1.0f))
        {
            JPH::ScaledShapeSettings scaledSettings(baseShape,
                                                    JPH::Vec3(desc.MeshScale.x, desc.MeshScale.y, desc.MeshScale.z));
            auto scaledResult = scaledSettings.Create();
            if (!scaledResult.HasError())
            {
                shape = scaledResult.Get();
            }
            else
            {
                CH_CORE_WARN("Physics: ScaledShape build failed: {} — using unscaled mesh.",
                             scaledResult.GetError().c_str());
            }
        }
        break;
    }
    default: {
        CH_CORE_WARN("Physics: Unknown ColliderType — falling back to unit box.");
        shape = JPH::BoxShapeSettings(JPH::Vec3(0.5f, 0.5f, 0.5f)).Create().Get();
        if (!shape)
        {
            CH_CORE_ERROR("Physics: unit-box fallback shape creation failed.");
        }
        break;
    }
    }

    // Apply Offset if present
    if (shape && desc.Offset != glm::vec3(0.0f))
    {
        JPH::RotatedTranslatedShapeSettings offsetSettings(JPH::Vec3(desc.Offset.x, desc.Offset.y, desc.Offset.z),
                                                           JPH::Quat::sIdentity(), shape);
        auto offsetResult = offsetSettings.Create();
        if (!offsetResult.HasError())
        {
            shape = offsetResult.Get();
        }
    }

    return shape;
}

JPH::BodyCreationSettings JoltPhysicsWorld::BuildBodySettings(const PhysicsBodyDesc& desc, JPH::ShapeRefC shape)
{
    JPH::EMotionType motionType;
    JPH::ObjectLayer objectLayer;

    if (desc.IsStatic)
    {
        motionType = JPH::EMotionType::Static;
        objectLayer = Layers::NON_MOVING;
    }
    else if (desc.IsKinematic)
    {
        motionType = JPH::EMotionType::Kinematic;
        objectLayer = Layers::MOVING;
    }
    else
    {
        motionType = JPH::EMotionType::Dynamic;
        objectLayer = Layers::MOVING;
    }

    JPH::BodyCreationSettings bodySettings(
        shape, JPH::RVec3(desc.Position.x, desc.Position.y, desc.Position.z),
        JPH::Quat(desc.Rotation.x, desc.Rotation.y, desc.Rotation.z, desc.Rotation.w), motionType, objectLayer);

    bodySettings.mFriction = desc.Friction;
    bodySettings.mRestitution = desc.Restitution;
    bodySettings.mLinearDamping = desc.LinearDamping;
    bodySettings.mAngularDamping = desc.AngularDamping;
    bodySettings.mAllowSleeping = true;

    if (desc.Shape == ColliderType::Mesh && !desc.IsStatic && !desc.Triangles.empty())
    {
        JPH::Vec3 bbMin(FLT_MAX, FLT_MAX, FLT_MAX), bbMax(-FLT_MAX, -FLT_MAX, -FLT_MAX);
        for (const auto& tri : desc.Triangles)
        {
            const JPH::Vec3 verts[] = {
                JPH::Vec3(tri.V0.x, tri.V0.y, tri.V0.z),
                JPH::Vec3(tri.V1.x, tri.V1.y, tri.V1.z),
                JPH::Vec3(tri.V2.x, tri.V2.y, tri.V2.z),
            };
            for (const auto& v : verts)
            {
                bbMin = JPH::Vec3::sMin(bbMin, v);
                bbMax = JPH::Vec3::sMax(bbMax, v);
            }
        }
        JPH::Vec3 boxSize = bbMax - bbMin;
        if (boxSize.GetX() > 0.0f && boxSize.GetY() > 0.0f && boxSize.GetZ() > 0.0f)
        {
            float volume = boxSize.GetX() * boxSize.GetY() * boxSize.GetZ();
            float density = (volume > 0.0f) ? (desc.Mass / volume) : 1.0f;
            bodySettings.mMassPropertiesOverride.SetMassAndInertiaOfSolidBox(boxSize, density);
            bodySettings.mOverrideMassProperties = JPH::EOverrideMassProperties::MassAndInertiaProvided;
        }
        else
        {
            bodySettings.mMassPropertiesOverride.mMass = desc.Mass;
            bodySettings.mMassPropertiesOverride.mInertia = JPH::Mat44::sScale(desc.Mass);
            bodySettings.mOverrideMassProperties = JPH::EOverrideMassProperties::MassAndInertiaProvided;
        }
    }
    else
    {
        bodySettings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateMassAndInertia;
    }

    if (desc.IsKinematic)
    {
        bodySettings.mGravityFactor = 0.0f;
    }
    else
    {
        bodySettings.mGravityFactor = desc.UseGravity ? 1.0f : 0.0f;
    }

    if (desc.IsFixedRotation)
    {
        bodySettings.mAllowedDOFs =
            JPH::EAllowedDOFs::TranslationX | JPH::EAllowedDOFs::TranslationY | JPH::EAllowedDOFs::TranslationZ;
    }

    bodySettings.mUserData = desc.UserData;

    return bodySettings;
}

PhysicsBodyHandle JoltPhysicsWorld::CreateBody(const PhysicsBodyDesc& desc)
{
    auto shape = BuildShape(desc);
    if (!shape)
    {
        return kInvalidPhysicsBody;
    }

    auto settings = BuildBodySettings(desc, shape);

    JPH::BodyInterface& bi = m_PhysicsSystem.GetBodyInterface();
    JPH::Body* body = bi.CreateBody(settings);
    if (!body)
    {
        CH_CORE_ERROR("Physics: Body creation failed (user-data {}).", desc.UserData);
        return kInvalidPhysicsBody;
    }
    bi.AddBody(body->GetID(), JPH::EActivation::Activate);

    return (PhysicsBodyHandle)body->GetID().GetIndexAndSequenceNumber();
}

std::vector<PhysicsBodyHandle> JoltPhysicsWorld::CreateBodies(const std::vector<PhysicsBodyDesc>& descs)
{
    if (descs.empty())
    {
        return {};
    }

    const auto buildStart = std::chrono::steady_clock::now();

    // Phase 0: Build all shapes in parallel (BVH build is the bottleneck)
    std::vector<JPH::ShapeRefC> prebuiltShapes(descs.size());
    {
        // Count mesh shapes that need building (non-trivial shapes)
        size_t meshCount = 0;
        for (const auto& desc : descs)
        {
            if (desc.Shape == ColliderType::Mesh && !desc.Triangles.empty())
            {
                ++meshCount;
            }
        }

        if (meshCount >= 2)
        {
            // Parallel path: build mesh shapes concurrently
            std::vector<std::future<void>> futures;
            futures.reserve(descs.size());

            for (size_t i = 0; i < descs.size(); ++i)
            {
                futures.push_back(std::async(std::launch::async, [this, &descs, &prebuiltShapes, i]() {
                    auto shape = BuildShape(descs[i]);
                    if (!shape)
                    {
                        shape = JPH::BoxShapeSettings(JPH::Vec3(0.5f, 0.5f, 0.5f)).Create().Get();
                    }
                    prebuiltShapes[i] = std::move(shape);
                }));
            }
            for (auto& f : futures)
            {
                f.get();
            }
        }
        else
        {
            // Sequential path: few shapes, no parallelism overhead
            for (size_t i = 0; i < descs.size(); ++i)
            {
                auto shape = BuildShape(descs[i]);
                if (!shape)
                {
                    shape = JPH::BoxShapeSettings(JPH::Vec3(0.5f, 0.5f, 0.5f)).Create().Get();
                }
                prebuiltShapes[i] = std::move(shape);
            }
        }
    }

    const auto shapesBuilt = std::chrono::steady_clock::now();
    const double shapesMs = std::chrono::duration<double, std::milli>(shapesBuilt - buildStart).count();
    if (shapesMs > 10.0)
    {
        CH_CORE_INFO("Physics: Built {} shapes in {:.1f} ms", descs.size(), shapesMs);
    }

    // Phase 1: Create bodies using pre-built shapes (fast, no BVH work)
    JPH::BodyInterface& bi = m_PhysicsSystem.GetBodyInterface();
    std::vector<JPH::BodyID> bodyIds;
    bodyIds.reserve(descs.size());

    for (size_t i = 0; i < descs.size(); ++i)
    {
        auto settings = BuildBodySettings(descs[i], prebuiltShapes[i]);
        JPH::Body* body = bi.CreateBody(settings);
        if (!body)
        {
            CH_CORE_ERROR("Physics: Batch body creation failed (user-data {}).", descs[i].UserData);
            bodyIds.push_back(JPH::BodyID());
            continue;
        }
        bodyIds.push_back(body->GetID());
    }

    // Phase 2: Batch add to physics system (avoids per-body broadphase rebuild)
    // Filter out invalid BodyIDs that failed creation — passing them to Jolt is UB.
    std::vector<JPH::BodyID> validIds;
    validIds.reserve(bodyIds.size());
    for (auto& id : bodyIds)
    {
        if (!id.IsInvalid())
        {
            validIds.push_back(id);
        }
    }

    if (!validIds.empty())
    {
        auto addState = bi.AddBodiesPrepare(validIds.data(), (int)validIds.size());
        bi.AddBodiesFinalize(validIds.data(), (int)validIds.size(), addState, JPH::EActivation::Activate);
    }

    // Phase 3: Convert to handles
    std::vector<PhysicsBodyHandle> handles;
    handles.reserve(bodyIds.size());
    for (auto& id : bodyIds)
    {
        if (id.IsInvalid())
        {
            handles.push_back(kInvalidPhysicsBody);
        }
        else
        {
            handles.push_back((PhysicsBodyHandle)id.GetIndexAndSequenceNumber());
        }
    }

    return handles;
}

void JoltPhysicsWorld::DestroyBody(PhysicsBodyHandle handle)
{
    JPH::BodyInterface& bi = m_PhysicsSystem.GetBodyInterface();
    JPH::BodyID id((JPH::uint32)handle);
    bi.RemoveBody(id);
    bi.DestroyBody(id);
}

bool JoltPhysicsWorld::HasCachedMeshShape(const std::string& key) const
{
    return !key.empty() && m_MeshShapeCache.count(key) > 0;
}

void JoltPhysicsWorld::SetTransform(PhysicsBodyHandle handle, const glm::vec3& pos, const glm::quat& rot)
{
    m_PhysicsSystem.GetBodyInterface().SetPositionAndRotation((JPH::BodyID)handle, JPH::RVec3(pos.x, pos.y, pos.z),
                                                              JPH::Quat(rot.x, rot.y, rot.z, rot.w),
                                                              JPH::EActivation::Activate);
}

void JoltPhysicsWorld::GetTransform(PhysicsBodyHandle handle, glm::vec3& pos, glm::quat& rot)
{
    JPH::RVec3 jPos;
    JPH::Quat jRot;
    m_PhysicsSystem.GetBodyInterface().GetPositionAndRotation((JPH::BodyID)handle, jPos, jRot);
    pos = {jPos.GetX(), jPos.GetY(), jPos.GetZ()};
    rot = {jRot.GetW(), jRot.GetX(), jRot.GetY(), jRot.GetZ()};
}

void JoltPhysicsWorld::SetVelocity(PhysicsBodyHandle handle, const glm::vec3& velocity)
{
    m_PhysicsSystem.GetBodyInterface().SetLinearVelocity((JPH::BodyID)handle,
                                                         JPH::Vec3(velocity.x, velocity.y, velocity.z));
}

glm::vec3 JoltPhysicsWorld::GetVelocity(PhysicsBodyHandle handle) const
{
    JPH::Vec3 v = m_PhysicsSystem.GetBodyInterface().GetLinearVelocity((JPH::BodyID)handle);
    return {v.GetX(), v.GetY(), v.GetZ()};
}

RaycastResult JoltPhysicsWorld::Raycast(const glm::vec3& origin, const glm::vec3& direction, float maxDistance)
{
    JPH::RRayCast ray{JPH::RVec3(origin.x, origin.y, origin.z),
                      JPH::Vec3(direction.x, direction.y, direction.z) * maxDistance};

    JPH::RayCastResult result;
    if (m_PhysicsSystem.GetNarrowPhaseQuery().CastRay(ray, result))
    {
        JPH::RVec3 hitPos = ray.GetPointOnRay(result.mFraction);
        RaycastResult out;
        out.Hit = true;
        out.Distance = result.mFraction * maxDistance;
        out.Position = {hitPos.GetX(), hitPos.GetY(), hitPos.GetZ()};
        out.BodyHandle = (PhysicsBodyHandle)result.mBodyID.GetIndexAndSequenceNumber();
        out.Entity = (entt::entity)m_PhysicsSystem.GetBodyInterface().GetUserData(result.mBodyID);
        return out;
    }
    return RaycastResult{false};
}

void JoltPhysicsWorld::Step(float fixedDt)
{
    m_PhysicsSystem.Update(fixedDt, 1, m_TempAllocator.get(), m_JobSystem.get());
}

void JoltPhysicsWorld::SetGravity(float gravity)
{
    m_PhysicsSystem.SetGravity(JPH::Vec3(0.0f, -gravity, 0.0f));
}

// ─────────────────────────────────────────────────────────────────────────────
// Ground detection via contact callbacks (O(1) lookup)
// ─────────────────────────────────────────────────────────────────────────────
bool JoltPhysicsWorld::IsBodyGrounded(PhysicsBodyHandle handle) const
{
    if (handle == 0)
    {
        return false;
    }
    std::lock_guard lock(m_GroundedMutex);
    return m_GroundedBodies.count(static_cast<uint32_t>(handle)) > 0;
}

bool JoltPhysicsWorld::IsBodyActive(PhysicsBodyHandle handle) const
{
    if (handle == 0)
    {
        return false;
    }
    return m_PhysicsSystem.GetBodyInterface().IsActive(JPH::BodyID(static_cast<uint32_t>(handle)));
}

void JoltPhysicsWorld::ClearGroundedState()
{
    std::lock_guard lock(m_GroundedMutex);
    std::unordered_set<uint32_t> nextGrounded;
    auto& bi = m_PhysicsSystem.GetBodyInterface();
    for (uint32_t handle : m_GroundedBodies)
    {
        if (!bi.IsActive(JPH::BodyID(handle)))
        {
            nextGrounded.insert(handle);
        }
    }
    m_GroundedBodies = std::move(nextGrounded);
}

// ─────────────────────────────────────────────────────────────────────────────
// ContactListenerImpl
// ─────────────────────────────────────────────────────────────────────────────
JPH::ValidateResult JoltPhysicsWorld::ContactListenerImpl::OnContactValidate(const JPH::Body&, const JPH::Body&,
                                                                             JPH::RVec3Arg,
                                                                             const JPH::CollideShapeResult&)
{
    return JPH::ValidateResult::AcceptAllContactsForThisBodyPair;
}

static void RegisterGroundContact(std::mutex& mutex, std::unordered_set<uint32_t>& tracker, const JPH::Body& body1,
                                  const JPH::Body& body2, const JPH::ContactManifold& manifold)
{
    JPH::Vec3 normal = manifold.mWorldSpaceNormal;
    uint32_t id1 = body1.GetID().GetIndexAndSequenceNumber();
    uint32_t id2 = body2.GetID().GetIndexAndSequenceNumber();

    std::lock_guard lock(mutex);
    // In Jolt, mWorldSpaceNormal points from body1 to body2.
    // If normal Y > 0.5, body2 is ABOVE body1. Thus body2 is grounded on body1.
    if (normal.GetY() > 0.5f)
    {
        tracker.insert(id2);
    }
    // If normal Y < -0.5, body1 is ABOVE body2. Thus body1 is grounded on body2.
    else if (normal.GetY() < -0.5f)
    {
        tracker.insert(id1);
    }
}

void JoltPhysicsWorld::ContactListenerImpl::OnContactAdded(const JPH::Body& body1, const JPH::Body& body2,
                                                           const JPH::ContactManifold& manifold, JPH::ContactSettings&)
{
    RegisterGroundContact(*m_Mutex, *m_Tracker, body1, body2, manifold);
}

void JoltPhysicsWorld::ContactListenerImpl::OnContactPersisted(const JPH::Body& body1, const JPH::Body& body2,
                                                               const JPH::ContactManifold& manifold,
                                                               JPH::ContactSettings&)
{
    RegisterGroundContact(*m_Mutex, *m_Tracker, body1, body2, manifold);
}

void JoltPhysicsWorld::ContactListenerImpl::OnContactRemoved(const JPH::SubShapeIDPair&)
{
    // Grounded set is rebuilt each frame by ClearGroundedState() + step contacts.
}

} // namespace Chained