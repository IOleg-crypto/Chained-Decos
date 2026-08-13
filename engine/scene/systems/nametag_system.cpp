#include "nametag_system.h"
#include "engine/core/service_locator.h"
#include "engine/graphics/pipeline/renderer.h"
#include "engine/graphics/pipeline/renderer_data.h"
#include "engine/graphics/api/graphics_device.h"
#include "engine/graphics/api/vertex_array.h"
#include "engine/graphics/api/buffer.h"
#include "engine/graphics/text_renderer.h"
#include "engine/assets/types/font_asset.h"
#include "engine/assets/asset_manager.h"
#include "engine/networking/network_service.h"
#include "engine/scene/components/gameplay/network_identity_component.h"
#include "engine/scene/components/core/transform_component.h"
#include "engine/scene/scene.h"
#include "engine/core/log.h"
#include <glm/glm.hpp>
#include <string>
#include <memory>

namespace Chained
{
	namespace NametagSystem
	{

		static TextRenderer s_TextRenderer;
		static std::shared_ptr<VertexArray> s_QuadVAO;
		static bool s_Initialized = false;

		static void EnsureInitialized()
		{
			if (s_Initialized)
			{
				return;
			}

			s_TextRenderer.Init();

			// Create a unit quad [0,1] x [0,1] with matching UVs
			float vertices[] = {
				// pos      // uv
				0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f,
			};
			uint32_t indices[] = {0, 1, 2, 2, 3, 0};

			auto vbo = VertexBuffer::Create(vertices, sizeof(vertices));
			vbo->SetLayout({{VertexAttributeType::Float2, "a_Position"}, {VertexAttributeType::Float2, "a_TexCoords"}});

			s_QuadVAO = VertexArray::Create();
			s_QuadVAO->AddVertexBuffer(vbo);
			auto ibo = IndexBuffer::Create(indices, 6);
			s_QuadVAO->SetIndexBuffer(ibo);

			s_Initialized = true;
		}

		void DrawNametags(Scene* scene, Renderer* renderer)
		{
			if (!scene || !renderer)
			{
				return;
			}

			auto* net = ServiceLocator::TryGet<Network>();
			if (!net || net->GetRole() == Role::Offline)
			{
				return;
			}

			EnsureInitialized();

			// Load the Nametag billboard shader via the Renderer's shader library
			auto& shaderStorage = renderer->GetShaderLibrary();
			auto shaderAsset =
				shaderStorage.LoadOrGet("Nametag", "engine/resources/shaders/materials/nametag.chshader");
			if (!shaderAsset || !shaderAsset->IsReady())
			{
				return;
			}
			auto shader = shaderAsset->GetShader();
			if (!shader)
			{
				return;
			}

			// Get a font for text rendering — "Default" is a sentinel that only
			// UIFontRegistry understands; AssetManager needs a real file path.
			auto* am = ServiceLocator::TryGet<AssetManager>();
			if (!am)
			{
				return;
			}

			auto fontAsset = am->Get<FontAsset>("font/lato/lato-regular.ttf");
			if (!fontAsset)
			{
				return;
			}
			const auto& font = fontAsset->GetFont();

			const auto& frame = renderer->GetFrame();
			const auto& players = net->GetPlayerList();
			uint64_t localID = net->GetLocalNetworkID();

			auto& reg = scene->GetRegistry();
			auto view = reg.view<NetworkIdentityComponent, TransformComponent>();

			shader->Bind();
			shader->SetMatrix("projection", frame.Proj);
			shader->SetMatrix("view", frame.View);
			shader->SetFloat("useBackground", 1.0f);
			shader->SetVec4("backgroundColor", glm::vec4(0.0f, 0.0f, 0.0f, 0.55f));

			GraphicsDevice::Get().SetTexture(0, 0); // ensure slot 0 is free

			PipelineStateGuard stateGuard;
			stateGuard.WithBlend().WithCullNone();

			s_QuadVAO->Bind();

			for (auto entity : view)
			{
				const auto& netId = view.get<NetworkIdentityComponent>(entity);
				const auto& tc = view.get<TransformComponent>(entity);

				// Skip local player — don't draw your own nametag
				if (netId.NetworkID == localID)
				{
					continue;
				}

				// Find player name
				std::string name;
				bool isHost = false;

				for (const auto& p : players)
				{
					if (p.NetworkID == netId.NetworkID)
					{
						name = p.Name;
						isHost = (p.IsHost != 0);
						break;
					}
				}
				if (name.empty())
				{
					continue;
				}

				// Build label: "Name" or "Name ★" for host
				std::string label = name;
				if (isHost)
				{
					label += " \xe2\x98\x85"; // UTF-8 ★
				}

				// Get or create text texture
				uint32_t texId = s_TextRenderer.GetOrCreateTexture(label, font, 32, 4);
				if (texId == 0)
				{
					continue;
				}

				int texW = s_TextRenderer.GetLastWidth();
				int texH = s_TextRenderer.GetLastHeight();
				if (texW <= 0 || texH <= 0)
				{
					continue;
				}

				// Billboard size: scale to world units based on texture aspect ratio
				float quadHeight = 0.35f;
				float quadWidth = quadHeight * (static_cast<float>(texW) / static_cast<float>(texH));

				// Position above avatar head
				glm::vec3 pos = tc.Translation + glm::vec3(0.0f, 2.2f, 0.0f);

				// Set per-nametag uniforms
				shader->SetVec3("modelPosition", pos);
				shader->SetVec2("size", glm::vec2(quadWidth, quadHeight));

				// Bind text texture and set color
				GraphicsDevice::Get().SetTexture(0, texId);
				shader->SetInt("textTexture", 0);

				bool isLocal = (netId.NetworkID == localID);
				shader->SetVec4("textColor", isLocal ? glm::vec4(1.0f, 0.86f, 0.0f, 1.0f)  // gold for local
													 : glm::vec4(1.0f, 1.0f, 1.0f, 0.9f)); // white for remote

				GraphicsDevice::Get().DrawIndexed(s_QuadVAO, 6);
			}

			s_QuadVAO->Unbind();
			shader->Unbind();
		}

		void Shutdown()
		{
			if (!s_Initialized)
			{
				return;
			}
			s_TextRenderer.Shutdown();
			s_QuadVAO.reset();
			s_Initialized = false;
		}

	} // namespace NametagSystem
} // namespace Chained
