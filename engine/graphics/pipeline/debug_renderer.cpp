#include "engine/graphics/pipeline/debug_renderer.h"
#include "engine/assets/asset_manager.h"
#include "engine/assets/types/shader_asset.h"
#include "engine/core/service_locator.h"
#include "engine/graphics/pipeline/geometry_generator.h"
#include "engine/graphics/api/graphics_device.h"
#include "engine/graphics/pipeline/renderer.h"
#include <glm/gtc/matrix_transform.hpp>

namespace Chained
{

	void DebugRenderer::Initialize()
	{
		m_Resources.UnitCubeModel = std::make_unique<Model>();
		m_Resources.UnitCubeModel->Meshes.push_back(GeometryGenerator::GenerateUnitCube());

		m_Resources.UnitSphereModel = std::make_unique<Model>();
		m_Resources.UnitSphereModel->Meshes.push_back(GeometryGenerator::GenerateSphere(1.0f, 32, 32));

		m_Resources.UnitCapsuleModel = std::make_unique<Model>();
		m_Resources.UnitCapsuleModel->Meshes.push_back(GeometryGenerator::GenerateCapsule(1.0f, 2.0f, 32, 32));

		m_Resources.WireCubeModel = std::make_unique<Model>();
		m_Resources.WireCubeModel->Meshes.push_back(GeometryGenerator::GenerateWireCube());
	}

	void DebugRenderer::Shutdown()
	{
	}

	void DebugRenderer::DrawLine(const glm::vec3& start, const glm::vec3& end, const glm::vec4& color)
	{
		m_Lines.Vertices.push_back({start, color});
		m_Lines.Vertices.push_back({end, color});
	}

	void DebugRenderer::Flush(Renderer& renderer)
	{
		if (m_Lines.Vertices.empty())
		{
			return;
		}

		const auto& frame = renderer.GetFrame();

		auto debugShader = renderer.GetShaderLibrary().LoadOrGet("ColliderDebug");
		if (!debugShader || !debugShader->GetShader())
		{
			m_Lines.Vertices.clear();
			return;
		}

		auto shader = debugShader->GetShader();
		shader->Bind();

		glm::mat4 vp = frame.Proj * frame.View;
		shader->SetMatrix("u_ViewProj", vp);
		shader->SetMatrix("u_Transform", glm::mat4(1.0f));
		shader->SetVec4("u_Color", glm::vec4(1.0f));

		uint32_t dataSize = (uint32_t)(m_Lines.Vertices.size() * sizeof(LineVertex));

		if (!m_Lines.VBO || m_Lines.VBOSize < dataSize)
		{
			m_Lines.VBOSize = std::max(dataSize, (uint32_t)(1024 * sizeof(LineVertex)));
			m_Lines.VBO = VertexBuffer::Create(m_Lines.VBOSize);
			m_Lines.VBO->SetLayout(
				{{VertexAttributeType::Float3, "vertexPosition"}, {VertexAttributeType::Float4, "vertexColor"}});
			m_Lines.VAO = VertexArray::Create();
			m_Lines.VAO->AddVertexBuffer(m_Lines.VBO);
		}

		m_Lines.VBO->SetData(m_Lines.Vertices.data(), dataSize);
		GraphicsDevice::Get().DrawLines(m_Lines.VAO, (uint32_t)m_Lines.Vertices.size());
		m_Lines.Vertices.clear();
	}

	void DebugRenderer::DrawMeshWire(const Mesh& mesh, const glm::vec4& color, const glm::mat4& transform,
									 Renderer& renderer, bool useWireframe)
	{
		const auto& frame = renderer.GetFrame();
		auto debugShader = renderer.GetShaderLibrary().LoadOrGet("ColliderDebug");
		if (!debugShader || !debugShader->GetShader())
		{
			return;
		}

		auto shader = debugShader->GetShader();
		shader->Bind();

		glm::mat4 vp = frame.Proj * frame.View;
		shader->SetMatrix("u_ViewProj", vp);
		shader->SetMatrix("u_Transform", transform);

		glm::vec4 finalColor = color;
		if (!useWireframe)
		{
			finalColor.a *= 0.35f;
		}
		shader->SetVec4("u_Color", finalColor);

		if (mesh.VAO)
		{
			auto guard = PipelineStateGuard::Capture();
			guard.WithBlend();
			GraphicsDevice::Get().SetBlendEnabled(true);
			GraphicsDevice::Get().SetBlendFunc(GraphicsDevice::BlendFactor::SrcAlpha,
											   GraphicsDevice::BlendFactor::OneMinusSrcAlpha);

			if (useWireframe)
			{
				GraphicsDevice::Get().SetPolygonMode(GraphicsDevice::PolygonMode::Line);
			}
			else
			{
				GraphicsDevice::Get().SetPolygonMode(GraphicsDevice::PolygonMode::Fill);
			}

			if (mesh.TriangleCount > 0)
			{
				GraphicsDevice::Get().DrawIndexed(mesh.VAO, mesh.TriangleCount * 3);
			}
			else if (mesh.VAO->GetIndexBuffer() != nullptr)
			{
				GraphicsDevice::Get().DrawIndexedLines(mesh.VAO, mesh.VAO->GetIndexBuffer()->GetCount());
			}
			else
			{
				GraphicsDevice::Get().DrawLines(mesh.VAO, mesh.VertexCount);
			}
		}
	}

	void DebugRenderer::DrawCubeWires(const glm::mat4& transform, const glm::vec3& size, const glm::vec4& color,
									  Renderer& renderer, bool useWireframe)
	{
		glm::mat4 model = transform * glm::scale(glm::mat4(1.0f), size);
		if (useWireframe && m_Resources.WireCubeModel && !m_Resources.WireCubeModel->Meshes.empty())
		{
			DrawMeshWire(m_Resources.WireCubeModel->Meshes[0], color, model, renderer, true);
		}
		else if (m_Resources.UnitCubeModel && !m_Resources.UnitCubeModel->Meshes.empty())
		{
			DrawMeshWire(m_Resources.UnitCubeModel->Meshes[0], color, model, renderer, useWireframe);
		}
	}

	void DebugRenderer::DrawCapsuleWires(const glm::mat4& transform, float radius, float height, const glm::vec4& color,
										 Renderer& renderer, bool useWireframe)
	{
		if (!m_Resources.UnitCapsuleModel)
		{
			return;
		}
		glm::mat4 model = transform * glm::scale(glm::mat4(1.0f), glm::vec3(radius, height, radius));
		for (auto& mesh : m_Resources.UnitCapsuleModel->Meshes)
		{
			DrawMeshWire(mesh, color, model, renderer, useWireframe);
		}
	}

	void DebugRenderer::DrawSphereWires(const glm::mat4& transform, float radius, const glm::vec4& color,
										Renderer& renderer, bool useWireframe)
	{
		if (!m_Resources.UnitSphereModel)
		{
			return;
		}
		glm::mat4 model = transform * glm::scale(glm::mat4(1.0f), glm::vec3(radius));
		for (auto& mesh : m_Resources.UnitSphereModel->Meshes)
		{
			DrawMeshWire(mesh, color, model, renderer, useWireframe);
		}
	}

	void DebugRenderer::DrawInfiniteGrid(const Camera3D& camera, float spacing, const glm::vec4& color,
										 Renderer& renderer)
	{
		const auto& frame = renderer.GetFrame();
		(void)frame;

		auto shaderAsset = renderer.GetShaderLibrary().LoadOrGet("Grid");
		if (!shaderAsset || !shaderAsset->GetShader())
		{
			return;
		}

		auto guard = PipelineStateGuard::Capture();
		GraphicsDevice::Get().SetCullMode(GraphicsDevice::CullMode::None);
		GraphicsDevice::Get().DisableDepthTest();
		GraphicsDevice::Get().SetBlendEnabled(true);
		GraphicsDevice::Get().SetBlendFunc(GraphicsDevice::BlendFactor::SrcAlpha,
										   GraphicsDevice::BlendFactor::OneMinusSrcAlpha);

		shaderAsset->GetShader()->Bind();

		glm::vec3 planePos = {camera.Position.x, -0.005f, camera.Position.z};
		glm::mat4 model = glm::translate(glm::mat4(1.0f), planePos);
		model = glm::scale(model, glm::vec3(15000.0f, 1.0f, 15000.0f));

		shaderAsset->GetShader()->SetMatrix("u_ViewProjection", frame.Proj * frame.View);
		shaderAsset->GetShader()->SetMatrix("u_Model", model);
		shaderAsset->GetShader()->SetVec3("u_CameraPos", camera.Position);
		shaderAsset->GetShader()->SetVec4("u_GridColor", color);
		shaderAsset->GetShader()->SetFloat("u_GridSize", spacing);
		shaderAsset->GetShader()->SetFloat("u_FadeStart", 0.0f);
		shaderAsset->GetShader()->SetFloat("u_FadeEnd", 8000.0f);

		if (!m_GridPlaneVAO)
		{
			float vertices[] = {
				-0.5f, 0.0f, -0.5f, 0.5f, 0.0f, -0.5f, 0.5f, 0.0f, 0.5f, -0.5f, 0.0f, 0.5f,
			};
			uint32_t indices[] = {0, 1, 2, 2, 3, 0};

			auto vbo = VertexBuffer::Create(vertices, sizeof(vertices));
			vbo->SetLayout({{VertexAttributeType::Float3, "a_Position"}});

			m_GridPlaneVAO = VertexArray::Create();
			m_GridPlaneVAO->AddVertexBuffer(vbo);
			auto ibo = IndexBuffer::Create(indices, 6);
			m_GridPlaneVAO->SetIndexBuffer(ibo);
		}

		GraphicsDevice::Get().DrawIndexed(m_GridPlaneVAO, 6);
	}

} // namespace Chained
