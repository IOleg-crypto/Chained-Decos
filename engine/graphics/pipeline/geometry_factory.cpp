#include "engine/graphics/pipeline/geometry_factory.h"
#include "engine/graphics/api/vertex_array.h"
#include "engine/graphics/api/buffer.h"

namespace Chained
{

	void GeometryFactory::Initialize()
	{
		// Fullscreen quad — position (vec3) + UV (vec2), used by post-processing
		{
			float vertices[] = {-1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f,  -1.0f, 0.0f, 1.0f, 0.0f,
								1.0f,  1.0f,  0.0f, 1.0f, 1.0f, -1.0f, 1.0f,  0.0f, 0.0f, 1.0f};
			uint32_t indices[] = {0, 1, 2, 2, 3, 0};

			m_FullscreenQuadVAO = VertexArray::Create();
			auto vbo = VertexBuffer::Create(vertices, sizeof(vertices));
			vbo->SetLayout(
				{{VertexAttributeType::Float3, "vertexPosition"}, {VertexAttributeType::Float2, "vertexTexCoord"}});
			m_FullscreenQuadVAO->AddVertexBuffer(vbo);
			auto ibo = IndexBuffer::Create(indices, 6);
			m_FullscreenQuadVAO->SetIndexBuffer(ibo);
		}

		// Billboard / Sprite quad — position (vec3) + UV (vec2) + normal (vec3)
		{
			float vertices[] = {
				// x,     y,     z,     u,    v,    nx,   ny,   nz
				-0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.5f,	 -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
				0.5f,  0.5f,  0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, -0.5f, 0.5f,	0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
			};
			uint32_t indices[] = {0, 1, 2, 2, 3, 0};

			auto vbo = VertexBuffer::Create(vertices, sizeof(vertices));
			vbo->SetLayout({{VertexAttributeType::Float3, "a_Position"},
							{VertexAttributeType::Float2, "a_TexCoord"},
							{VertexAttributeType::Float3, "a_Normal"}});

			m_QuadVAO = VertexArray::Create();
			m_QuadVAO->AddVertexBuffer(vbo);
			auto ibo = IndexBuffer::Create(indices, 6);
			m_QuadVAO->SetIndexBuffer(ibo);
		}
	}

	void GeometryFactory::Shutdown()
	{
		m_FullscreenQuadVAO.reset();
		m_QuadVAO.reset();
	}

} // namespace Chained
