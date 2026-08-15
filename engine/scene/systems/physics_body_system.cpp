#include "physics_body_system.h"
#include "engine/assets/asset_manager.h"
#include "engine/assets/types/model_asset.h"
#include "engine/core/profiler.h"
#include "engine/core/service_locator.h"
#include "engine/physics/iphysics_world.h"
#include "engine/physics/physics.h"
#include "engine/scene/components/physics/physics_component.h"
#include "engine/scene/components/render/model_component.h"
#include "engine/scene/components/core/transform_component.h"
#include "engine/common/thread_pool.h"
#include <algorithm>
#include <future>

namespace Chained::PhysicsBodySystem
{

	void ApplyAutoCalculate(entt::entity entity, entt::registry& registry, ColliderComponent& collider,
							const glm::vec3& scale)
	{
		std::string modelPath = collider.ModelPath;
		if (modelPath.empty())
		{
			if (auto* mc = registry.try_get<ModelComponent>(entity))
			{
				modelPath = mc->ModelPath;
			}
		}

		if (modelPath.empty())
		{
			CH_CORE_WARN("PhysicsBodySystem::ApplyAutoCalculate: no model path found for entity={}", (uint32_t)entity);
			return;
		}

		auto* am = ServiceLocator::TryGet<AssetManager>();
		if (!am)
		{
			return;
		}

		auto handle = am->ResolveToHandle(modelPath);
		if (handle == AssetHandle(0))
		{
			CH_CORE_WARN("PhysicsBodySystem::ApplyAutoCalculate: model '{}' not loaded.", modelPath);
			return;
		}

		auto asset = am->Get<ModelAsset>(handle);
		if (!asset || asset->GetState() != AssetState::Ready)
		{
			CH_CORE_WARN("PhysicsBodySystem::ApplyAutoCalculate: model '{}' not ready.", modelPath);
			return;
		}

		const auto& bbox = asset->GetBoundingBox();
		glm::vec3 bMin = bbox.Min * scale;
		glm::vec3 bMax = bbox.Max * scale;

		glm::vec3 size = bMax - bMin;
		glm::vec3 center = (bMax + bMin) * 0.5f;

		collider.Size = size;
		collider.Radius = glm::compMax(size) * 0.5f;
		collider.Height = size.y;

		if (collider.Type != ColliderType::Mesh)
		{
			collider.Offset = center;
		}

		CH_CORE_INFO("PhysicsBodySystem::ApplyAutoCalculate: entity={} model='{}' → Size=({:.2f},{:.2f},{:.2f}) "
					 "Offset=({:.2f},{:.2f},{:.2f}) Radius={:.2f} Height={:.2f}",
					 (uint32_t)entity, modelPath, collider.Size.x, collider.Size.y, collider.Size.z, collider.Offset.x,
					 collider.Offset.y, collider.Offset.z, collider.Radius, collider.Height);
	}

	bool BuildBodyDesc(entt::registry& reg, entt::entity e, PhysicsBodyDesc& outDesc)
	{
		if (!reg.all_of<TransformComponent, RigidBodyComponent>(e))
		{
			return false;
		}

		auto& transform = reg.get<TransformComponent>(e);
		auto& rb = reg.get<RigidBodyComponent>(e);

		auto* collider = reg.try_get<ColliderComponent>(e);
		if (!collider || !collider->Enabled)
		{
			return false;
		}

		PhysicsBodyDesc desc;
		desc.Position = transform.Translation;
		desc.Rotation = transform.RotationQuat;
		desc.IsKinematic = (rb.Type == RigidBodyComponent::BodyType::Kinematic);
		desc.IsStatic = (rb.Type == RigidBodyComponent::BodyType::Static);
		desc.Mass = rb.Mass;
		desc.LinearDamping = rb.LinearDamping;
		desc.AngularDamping = rb.AngularDamping;
		desc.UseGravity = rb.UseGravity;
		desc.IsFixedRotation = rb.IsFixedRotation;
		desc.InitialVelocity = rb.Velocity;
		desc.UserData = (uint64_t)e;

		desc.Shape = collider->Type;
		desc.Friction = collider->Friction;
		desc.Restitution = collider->Restitution;
		desc.Offset = collider->Offset;
		desc.UseFastBuildQuality = collider->UseFastBuildQuality;

		switch (collider->Type)
		{
		case ColliderType::Box:
			desc.Dimensions = (collider->Size * transform.Scale) * 0.5f;
			break;

		case ColliderType::Sphere:
			desc.Dimensions.x = collider->Radius * std::max({transform.Scale.x, transform.Scale.y, transform.Scale.z});
			break;

		case ColliderType::Capsule:
			desc.Dimensions.x = collider->Radius * std::max(transform.Scale.x, transform.Scale.z);
			desc.Dimensions.y = (collider->Height * transform.Scale.y) * 0.5f;
			break;

		case ColliderType::Mesh: {
			std::string modelPath = collider->ModelPath;
			if (modelPath.empty())
			{
				if (auto* modelComp = reg.try_get<ModelComponent>(e))
				{
					modelPath = modelComp->ModelPath;
				}
			}

			if (modelPath.empty())
			{
				return false;
			}

			if (auto* physicsPtr = reg.ctx().find<Physics*>())
			{
				if (*physicsPtr && (*physicsPtr)->GetWorld() &&
					(*physicsPtr)->GetWorld()->HasCachedMeshShape(modelPath))
				{
					desc.CacheKey = modelPath;
					desc.MeshScale = transform.Scale;
					break;
				}
			}

			auto* assets = ServiceLocator::TryGet<AssetManager>();
			if (!assets)
			{
				return false;
			}

			auto modelAsset = assets->Get<ModelAsset>(modelPath);
			if (!modelAsset || !modelAsset->IsReady())
			{
				return false;
			}

			const auto& rawMeshes = modelAsset->GetMeshes();
			const auto& instances = modelAsset->GetInstances();

			desc.MeshScale = transform.Scale;
			desc.CacheKey = modelPath;

			for (const auto& inst : instances)
			{
				if (inst.meshIndex < 0 || inst.meshIndex >= (int)rawMeshes.size())
				{
					continue;
				}

				const MeshData& raw = rawMeshes[inst.meshIndex];
				if (raw.indices.size() < 3)
				{
					continue;
				}

				const glm::mat4& meshToLocal = inst.localTransform;

				for (size_t i = 0; i + 2 < raw.indices.size(); i += 3)
				{
					uint32_t i0 = raw.indices[i];
					uint32_t i1 = raw.indices[i + 1];
					uint32_t i2 = raw.indices[i + 2];

					size_t v0Idx = (size_t)i0 * 3;
					size_t v1Idx = (size_t)i1 * 3;
					size_t v2Idx = (size_t)i2 * 3;

					if (v0Idx + 2 >= raw.vertices.size() || v1Idx + 2 >= raw.vertices.size() ||
						v2Idx + 2 >= raw.vertices.size())
					{
						continue;
					}

					glm::vec3 v0 = {raw.vertices[v0Idx], raw.vertices[v0Idx + 1], raw.vertices[v0Idx + 2]};
					glm::vec3 v1 = {raw.vertices[v1Idx], raw.vertices[v1Idx + 1], raw.vertices[v1Idx + 2]};
					glm::vec3 v2 = {raw.vertices[v2Idx], raw.vertices[v2Idx + 1], raw.vertices[v2Idx + 2]};

					PhysicsTriangle tri;
					tri.V0 = glm::vec3(meshToLocal * glm::vec4(v0, 1.0f));
					tri.V1 = glm::vec3(meshToLocal * glm::vec4(v1, 1.0f));
					tri.V2 = glm::vec3(meshToLocal * glm::vec4(v2, 1.0f));

					desc.Triangles.push_back(tri);
				}
			}
			break;
		}
		}

		outDesc = std::move(desc);
		return true;
	}

	void TryCreateBody(entt::registry& reg, entt::entity e)
	{
		if (!reg.ctx().contains<Physics*>())
		{
			CH_CORE_WARN("Physics: TryCreateBody entity={} — no Physics* in context yet.", (uint32_t)e);
			return;
		}

		auto* physicsPtr = reg.ctx().find<Physics*>();
		if (!physicsPtr || !(*physicsPtr) || !(*physicsPtr)->GetWorld())
		{
			CH_CORE_WARN("Physics: TryCreateBody entity={} — Physics* is null or world not initialized.", (uint32_t)e);
			return;
		}

		auto* world = (*physicsPtr)->GetWorld();

		auto& rb = reg.get<RigidBodyComponent>(e);

		if (rb.Handle != kInvalidPhysicsBody)
		{
			return;
		}

		auto* collider = reg.try_get<ColliderComponent>(e);
		if (!collider || !collider->Enabled)
		{
			CH_CORE_WARN("Physics: TryCreateBody entity={} — ColliderComponent missing or disabled (has={}).",
						 (uint32_t)e, collider != nullptr);
			return;
		}

		if (collider->AutoCalculate)
		{
			auto& transform = reg.get<TransformComponent>(e);
			ApplyAutoCalculate(e, reg, *collider, transform.Scale);
		}

		PhysicsBodyDesc desc;
		if (!BuildBodyDesc(reg, e, desc))
		{
			CH_CORE_WARN("Physics: TryCreateBody entity={} — BuildBodyDesc failed.", (uint32_t)e);
			return;
		}

		rb.Handle = world->CreateBody(desc);
		if (rb.Handle == kInvalidPhysicsBody)
		{
			CH_CORE_ERROR("Physics: TryCreateBody entity={} — CreateBody returned invalid handle.", (uint32_t)e);
		}
		else
		{
			CH_CORE_INFO("Physics: TryCreateBody entity={} — body created (handle={}, type={}, mass={})", (uint32_t)e,
						 (uint64_t)rb.Handle, (int)rb.Type, rb.Mass);
		}
	}

	void BatchInitializeBodies(entt::registry& reg, IPhysicsWorld* world)
	{
		std::vector<PhysicsBodyDesc> descs;
		std::vector<entt::entity> entities;

		auto view = reg.view<RigidBodyComponent, TransformComponent>();
		for (auto entity : view)
		{
			auto& rb = view.get<RigidBodyComponent>(entity);
			if (rb.Handle != kInvalidPhysicsBody)
			{
				continue;
			}

			auto* collider = reg.try_get<ColliderComponent>(entity);
			if (!collider || !collider->Enabled)
			{
				CH_CORE_WARN("Physics: BatchInitializeBodies entity={} — ColliderComponent missing or disabled.",
							 (uint32_t)entity);
				continue;
			}

			CH_CORE_INFO("Physics: BatchInitializeBodies entity={} — Type={}, IsStatic={}, IsKinematic={}",
						 (uint32_t)entity, (int)rb.Type, (rb.Type == RigidBodyComponent::BodyType::Static),
						 (rb.Type == RigidBodyComponent::BodyType::Kinematic));

			if (collider->AutoCalculate)
			{
				ApplyAutoCalculate(entity, reg, *collider, view.get<TransformComponent>(entity).Scale);
			}

			PhysicsBodyDesc desc;
			if (!BuildBodyDesc(reg, entity, desc))
			{
				continue;
			}

			entities.push_back(entity);
			descs.push_back(std::move(desc));
		}

		if (descs.empty())
		{
			return;
		}

		// Sort: static (0) first, then kinematic (1), then dynamic (2)
		std::vector<size_t> indices(descs.size());
		for (size_t i = 0; i < indices.size(); ++i)
		{
			indices[i] = i;
		}
		std::sort(indices.begin(), indices.end(), [&descs](size_t a, size_t b) {
			int orderA = descs[a].IsStatic ? 0 : (descs[a].IsKinematic ? 1 : 2);
			int orderB = descs[b].IsStatic ? 0 : (descs[b].IsKinematic ? 1 : 2);
			return orderA < orderB;
		});

		// Reorder descs and entities by sort order
		std::vector<PhysicsBodyDesc> sortedDescs;
		std::vector<entt::entity> sortedEntities;
		sortedDescs.reserve(descs.size());
		sortedEntities.reserve(entities.size());
		for (size_t i : indices)
		{
			sortedDescs.push_back(std::move(descs[i]));
			sortedEntities.push_back(entities[i]);
		}

		// Pre-build un-cached mesh shapes in parallel on worker threads
		if (auto* tp = ServiceLocator::TryGet<ThreadPool>())
		{
			std::vector<std::future<void>> futures;
			for (const auto& desc : sortedDescs)
			{
				if (desc.Shape == ColliderType::Mesh && !desc.CacheKey.empty() &&
					!world->HasCachedMeshShape(desc.CacheKey))
				{
					futures.push_back(tp->Enqueue([world, desc]() { world->PrebuildShape(desc); }));
				}
			}
			for (auto& f : futures)
			{
				f.get();
			}
		}

		auto handles = world->CreateBodies(sortedDescs);

		for (size_t i = 0; i < sortedEntities.size(); ++i)
		{
			reg.get<RigidBodyComponent>(sortedEntities[i]).Handle = handles[i];
		}

		CH_CORE_INFO(
			"Physics: Batch-created {} bodies (static={}, dynamic={}, kinematic={}).", sortedEntities.size(),
			std::count_if(sortedDescs.begin(), sortedDescs.end(), [](const PhysicsBodyDesc& d) { return d.IsStatic; }),
			std::count_if(sortedDescs.begin(), sortedDescs.end(),
						  [](const PhysicsBodyDesc& d) { return !d.IsStatic && !d.IsKinematic; }),
			std::count_if(sortedDescs.begin(), sortedDescs.end(),
						  [](const PhysicsBodyDesc& d) { return d.IsKinematic; }));
	}

	void Update(entt::registry& reg)
	{
		CH_PROFILE_FUNCTION();

		if (!reg.ctx().contains<Physics*>())
		{
			return;
		}

		auto* physics = *reg.ctx().find<Physics*>();
		if (!physics || !physics->GetWorld())
		{
			return;
		}

		BatchInitializeBodies(reg, physics->GetWorld());
	}

} // namespace Chained::PhysicsBodySystem
