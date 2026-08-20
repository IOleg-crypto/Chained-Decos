#include "geometry_generator.h"
#include <vector>
#include <cmath>
#include <algorithm>
#include <limits>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

namespace Chained
{
	Mesh GeometryGenerator::GenerateUnitCube()
	{
		float vertices[] = {-0.5f, 0.5f,  -0.5f, -0.5f, -0.5f, -0.5f, 0.5f,	 -0.5f, -0.5f,
							0.5f,  -0.5f, -0.5f, 0.5f,	0.5f,  -0.5f, -0.5f, 0.5f,	-0.5f,

							-0.5f, -0.5f, 0.5f,	 -0.5f, -0.5f, -0.5f, -0.5f, 0.5f,	-0.5f,
							-0.5f, 0.5f,  0.5f,	 -0.5f, 0.5f,  0.5f,  -0.5f, -0.5f, 0.5f,

							0.5f,  -0.5f, -0.5f, 0.5f,	-0.5f, 0.5f,  0.5f,	 0.5f,	0.5f,
							0.5f,  0.5f,  0.5f,	 0.5f,	0.5f,  -0.5f, 0.5f,	 -0.5f, -0.5f,

							-0.5f, -0.5f, 0.5f,	 -0.5f, 0.5f,  0.5f,  0.5f,	 0.5f,	0.5f,
							0.5f,  0.5f,  0.5f,	 0.5f,	-0.5f, 0.5f,  -0.5f, -0.5f, 0.5f,

							-0.5f, 0.5f,  -0.5f, 0.5f,	0.5f,  -0.5f, 0.5f,	 0.5f,	0.5f,
							0.5f,  0.5f,  0.5f,	 -0.5f, 0.5f,  0.5f,  -0.5f, 0.5f,	-0.5f,

							-0.5f, -0.5f, -0.5f, -0.5f, -0.5f, 0.5f,  0.5f,	 -0.5f, -0.5f,
							0.5f,  -0.5f, -0.5f, -0.5f, -0.5f, 0.5f,  0.5f,	 -0.5f, 0.5f};

		auto vbo = VertexBuffer::Create(vertices, sizeof(vertices));
		vbo->SetLayout({{VertexAttributeType::Float3, "a_Position"}});
		auto vao = VertexArray::Create();
		vao->AddVertexBuffer(vbo);

		uint32_t indices[36];
		for (uint32_t index = 0; index < 36; ++index)
		{
			indices[index] = index;
		}
		auto ebo = IndexBuffer::Create(indices, 36);
		vao->SetIndexBuffer(ebo);

		Mesh mesh;
		mesh.VAO = vao;
		mesh.VertexCount = 36;
		mesh.TriangleCount = 12;
		return mesh;
	}

	Mesh GeometryGenerator::GenerateWireCube()
	{
		// 8 unique corner vertices of a unit cube [-0.5, 0.5]
		float vertices[] = {
			-0.5f, -0.5f, -0.5f, // 0: left  bottom back
			0.5f,  -0.5f, -0.5f, // 1: right bottom back
			0.5f,  0.5f,  -0.5f, // 2: right top    back
			-0.5f, 0.5f,  -0.5f, // 3: left  top    back
			-0.5f, -0.5f, 0.5f,	 // 4: left  bottom front
			0.5f,  -0.5f, 0.5f,	 // 5: right bottom front
			0.5f,  0.5f,  0.5f,	 // 6: right top    front
			-0.5f, 0.5f,  0.5f	 // 7: left  top    front
		};

		// 12 edges × 2 indices = 24 indices for GL_LINES
		uint32_t indices[] = {// Back face edges
							  0, 1, 1, 2, 2, 3, 3, 0,
							  // Front face edges
							  4, 5, 5, 6, 6, 7, 7, 4,
							  // Connecting edges
							  0, 4, 1, 5, 2, 6, 3, 7};

		auto vbo = VertexBuffer::Create(vertices, sizeof(vertices));
		vbo->SetLayout({{VertexAttributeType::Float3, "a_Position"}});
		auto vao = VertexArray::Create();
		vao->AddVertexBuffer(vbo);
		auto ebo = IndexBuffer::Create(indices, 24);
		vao->SetIndexBuffer(ebo);

		Mesh mesh;
		mesh.VAO = vao;
		mesh.VertexCount = 8;
		mesh.TriangleCount = 0; // Lines, not triangles
		return mesh;
	}

	Mesh GeometryGenerator::GenerateSphere(float radius, int slices, int stacks)
	{
		std::vector<float> vertices;
		std::vector<uint32_t> indices;

		for (int stackIndex = 0; stackIndex <= stacks; ++stackIndex)
		{
			float stackFraction = (float)stackIndex / (float)stacks;
			float polarAngle = stackFraction * glm::pi<float>();

			for (int sliceIndex = 0; sliceIndex <= slices; ++sliceIndex)
			{
				float sliceFraction = (float)sliceIndex / (float)slices;
				float azimuthAngle = sliceFraction * 2.0f * glm::pi<float>();

				float nx = std::cos(azimuthAngle) * std::sin(polarAngle);
				float ny = std::cos(polarAngle);
				float nz = std::sin(azimuthAngle) * std::sin(polarAngle);

				// Position
				vertices.push_back(nx * radius);
				vertices.push_back(ny * radius);
				vertices.push_back(nz * radius);
				// TexCoord
				vertices.push_back(sliceFraction);
				vertices.push_back(1.0f - stackFraction);
				// Normal
				vertices.push_back(nx);
				vertices.push_back(ny);
				vertices.push_back(nz);
			}
		}

		for (int stackIndex = 0; stackIndex < stacks; ++stackIndex)
		{
			for (int sliceIndex = 0; sliceIndex < slices; ++sliceIndex)
			{
				indices.push_back((stackIndex + 1) * (slices + 1) + sliceIndex);
				indices.push_back(stackIndex * (slices + 1) + sliceIndex);
				indices.push_back(stackIndex * (slices + 1) + sliceIndex + 1);
				indices.push_back((stackIndex + 1) * (slices + 1) + sliceIndex);
				indices.push_back(stackIndex * (slices + 1) + sliceIndex + 1);
				indices.push_back((stackIndex + 1) * (slices + 1) + (sliceIndex + 1));
			}
		}

		auto vbo = VertexBuffer::Create(vertices.data(), (uint32_t)vertices.size() * sizeof(float));
		// Pos(3) + Tex(2) + Normal(3) = 8 floats per vertex
		vbo->SetLayout({{VertexAttributeType::Float3, "a_Position"},
						{VertexAttributeType::Float2, "a_TexCoord"},
						{VertexAttributeType::Float3, "a_Normal"}});
		auto vao = VertexArray::Create();
		vao->AddVertexBuffer(vbo);
		auto ebo = IndexBuffer::Create(indices.data(), (uint32_t)indices.size());
		vao->SetIndexBuffer(ebo);

		Mesh mesh;
		mesh.VAO = vao;
		mesh.VertexCount = (uint32_t)vertices.size() / 8;
		mesh.TriangleCount = (uint32_t)indices.size() / 3;
		return mesh;
	}

	Mesh GeometryGenerator::GenerateGrid(int slices, float spacing)
	{
		std::vector<float> vertices;
		for (int lineIndex = -slices; lineIndex <= slices; ++lineIndex)
		{
			vertices.push_back((float)lineIndex * spacing);
			vertices.push_back(0);
			vertices.push_back((float)-slices * spacing);
			vertices.push_back((float)lineIndex * spacing);
			vertices.push_back(0);
			vertices.push_back((float)slices * spacing);

			vertices.push_back((float)-slices * spacing);
			vertices.push_back(0);
			vertices.push_back((float)lineIndex * spacing);
			vertices.push_back((float)slices * spacing);
			vertices.push_back(0);
			vertices.push_back((float)lineIndex * spacing);
		}

		auto vbo = VertexBuffer::Create(vertices.data(), (uint32_t)vertices.size() * sizeof(float));
		vbo->SetLayout({{VertexAttributeType::Float3, "a_Position"}});
		auto vao = VertexArray::Create();
		vao->AddVertexBuffer(vbo);

		Mesh mesh;
		mesh.VAO = vao;
		mesh.VertexCount = (uint32_t)vertices.size() / 3;
		mesh.TriangleCount = 0; // Grid is lines
		return mesh;
	}

	Mesh GeometryGenerator::GenerateQuad(float size)
	{
		// Pos(3), Tex(2), Normal(3)
		float vertices[] = {-size, 0.0f, -size, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, size, 0.0f, -size,
							1.0f,  0.0f, 0.0f,	1.0f, 0.0f, size, 0.0f, size, 1.0f, 1.0f, 0.0f,
							1.0f,  0.0f, -size, 0.0f, size, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f};

		auto vbo = VertexBuffer::Create(vertices, sizeof(vertices));
		vbo->SetLayout({{VertexAttributeType::Float3, "a_Position"},
						{VertexAttributeType::Float2, "a_TexCoord"},
						{VertexAttributeType::Float3, "a_Normal"}});
		auto vao = VertexArray::Create();
		vao->AddVertexBuffer(vbo);
		// TriangleCount = 2 below makes DrawMesh take the DrawIndexed path, so the VAO MUST
		// carry an index buffer — without it glDrawElements reads from a null EBO and crashes.
		uint32_t indices[] = {0, 1, 2, 2, 3, 0};
		auto ebo = IndexBuffer::Create(indices, 6);
		vao->SetIndexBuffer(ebo);

		Mesh mesh;
		mesh.VAO = vao;
		mesh.VertexCount = 4;
		mesh.TriangleCount = 2;
		return mesh;
	}

	Mesh GeometryGenerator::GenerateCube(const glm::vec3& dimensions)
	{
		float w = dimensions.x * 0.5f;
		float h = dimensions.y * 0.5f;
		float d = dimensions.z * 0.5f;

		// Front, Back, Top, Bottom, Right, Left faces
		// 8 floats per vertex: Pos(3), Tex(2), Normal(3)
		float vertices[] = {// Front
							-w, -h, d, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, w, -h, d, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, w, h, d,
							1.0f, 1.0f, 0.0f, 0.0f, 1.0f, -w, h, d, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
							// Back
							w, -h, -d, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, -w, -h, -d, 1.0f, 0.0f, 0.0f, 0.0f, -1.0f, -w, h,
							-d, 1.0f, 1.0f, 0.0f, 0.0f, -1.0f, w, h, -d, 0.0f, 1.0f, 0.0f, 0.0f, -1.0f,
							// Top
							-w, h, d, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, w, h, d, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, w, h, -d,
							1.0f, 1.0f, 0.0f, 1.0f, 0.0f, -w, h, -d, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,
							// Bottom
							-w, -h, -d, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, w, -h, -d, 1.0f, 0.0f, 0.0f, -1.0f, 0.0f, w, -h,
							d, 1.0f, 1.0f, 0.0f, -1.0f, 0.0f, -w, -h, d, 0.0f, 1.0f, 0.0f, -1.0f, 0.0f,
							// Right
							w, -h, d, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, w, -h, -d, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, w, h, -d,
							1.0f, 1.0f, 1.0f, 0.0f, 0.0f, w, h, d, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f,
							// Left
							-w, -h, -d, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -w, -h, d, 1.0f, 0.0f, -1.0f, 0.0f, 0.0f, -w, h,
							d, 1.0f, 1.0f, -1.0f, 0.0f, 0.0f, -w, h, -d, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f};

		uint32_t indices[] = {0,  1,  2,  2,  3,  0,  4,  5,  6,  6,  7,  4,  8,  9,  10, 10, 11, 8,
							  12, 13, 14, 14, 15, 12, 16, 17, 18, 18, 19, 16, 20, 21, 22, 22, 23, 20};

		auto vbo = VertexBuffer::Create(vertices, sizeof(vertices));
		vbo->SetLayout({{VertexAttributeType::Float3, "a_Position"},
						{VertexAttributeType::Float2, "a_TexCoord"},
						{VertexAttributeType::Float3, "a_Normal"}});
		auto vao = VertexArray::Create();
		vao->AddVertexBuffer(vbo);
		auto ebo = IndexBuffer::Create(indices, 36);
		vao->SetIndexBuffer(ebo);

		Mesh mesh;
		mesh.VAO = vao;
		mesh.VertexCount = 24;
		mesh.TriangleCount = 12;
		return mesh;
	}

	Mesh GeometryGenerator::GenerateCapsule(float radius, float height, int slices, int stacks)
	{
		std::vector<float> vertices;
		std::vector<uint32_t> indices;

		// A capsule is a cylinder with two hemispheres.
		// Height provided by user is total height.
		// Cylinder height = height - 2 * radius.
		float cylinderHeight = std::max(0.0f, height - 2.0f * radius);
		float halfCylinderHeight = cylinderHeight * 0.5f;

		// Generate vertices
		for (int stackIndex = 0; stackIndex <= stacks; ++stackIndex)
		{
			float stackFraction = (float)stackIndex / (float)stacks;
			float polarAngle = stackFraction * glm::pi<float>();

			// Offset Y based on which hemisphere or cylinder part we are in
			float yOffset = 0.0f;
			if (stackFraction < 0.5f)
			{
				yOffset = halfCylinderHeight;
			}
			else
			{
				yOffset = -halfCylinderHeight;
			}

			for (int sliceIndex = 0; sliceIndex <= slices; ++sliceIndex)
			{
				float sliceFraction = (float)sliceIndex / (float)slices;
				float azimuthAngle = sliceFraction * 2.0f * glm::pi<float>();

				float x = std::cos(azimuthAngle) * std::sin(polarAngle);
				float y = std::cos(polarAngle);
				float z = std::sin(azimuthAngle) * std::sin(polarAngle);

				vertices.push_back(x * radius);
				vertices.push_back(y * radius + yOffset);
				vertices.push_back(z * radius);
			}
		}

		// Generate indices
		for (int stackIndex = 0; stackIndex < stacks; ++stackIndex)
		{
			for (int sliceIndex = 0; sliceIndex < slices; ++sliceIndex)
			{
				indices.push_back((stackIndex + 1) * (slices + 1) + sliceIndex);
				indices.push_back(stackIndex * (slices + 1) + sliceIndex);
				indices.push_back(stackIndex * (slices + 1) + sliceIndex + 1);
				indices.push_back((stackIndex + 1) * (slices + 1) + sliceIndex);
				indices.push_back(stackIndex * (slices + 1) + sliceIndex + 1);
				indices.push_back((stackIndex + 1) * (slices + 1) + (sliceIndex + 1));
			}
		}

		auto vbo = VertexBuffer::Create(vertices.data(), (uint32_t)vertices.size() * sizeof(float));
		vbo->SetLayout({{VertexAttributeType::Float3, "a_Position"}});
		auto vao = VertexArray::Create();
		vao->AddVertexBuffer(vbo);
		auto ebo = IndexBuffer::Create(indices.data(), (uint32_t)indices.size());
		vao->SetIndexBuffer(ebo);

		Mesh mesh;
		mesh.VAO = vao;
		mesh.VertexCount = (uint32_t)vertices.size() / 3;
		mesh.TriangleCount = (uint32_t)indices.size() / 3;
		return mesh;
	}

	namespace
	{
		// Builds a MeshData (de-interleaved arrays, no VAO) for a box of the given dimensions.
		// Matches the interleaved layout of GenerateCube: 24 verts, per-face normals + texcoords.
		MeshData BuildCube(const glm::vec3& dimensions)
		{
			float w = dimensions.x * 0.5f;
			float h = dimensions.y * 0.5f;
			float d = dimensions.z * 0.5f;

			struct V
			{
				glm::vec3 p, n;
				glm::vec2 t;
			};
			const V verts[] = {
				// Front (+Z)
				{{-w, -h, d}, {0, 0, 1}, {0, 0}},
				{{w, -h, d}, {0, 0, 1}, {1, 0}},
				{{w, h, d}, {0, 0, 1}, {1, 1}},
				{{-w, h, d}, {0, 0, 1}, {0, 1}},
				// Back (-Z)
				{{w, -h, -d}, {0, 0, -1}, {0, 0}},
				{{-w, -h, -d}, {0, 0, -1}, {1, 0}},
				{{-w, h, -d}, {0, 0, -1}, {1, 1}},
				{{w, h, -d}, {0, 0, -1}, {0, 1}},
				// Top (+Y)
				{{-w, h, d}, {0, 1, 0}, {0, 0}},
				{{w, h, d}, {0, 1, 0}, {1, 0}},
				{{w, h, -d}, {0, 1, 0}, {1, 1}},
				{{-w, h, -d}, {0, 1, 0}, {0, 1}},
				// Bottom (-Y)
				{{-w, -h, -d}, {0, -1, 0}, {0, 0}},
				{{w, -h, -d}, {0, -1, 0}, {1, 0}},
				{{w, -h, d}, {0, -1, 0}, {1, 1}},
				{{-w, -h, d}, {0, -1, 0}, {0, 1}},
				// Right (+X)
				{{w, -h, d}, {1, 0, 0}, {0, 0}},
				{{w, -h, -d}, {1, 0, 0}, {1, 0}},
				{{w, h, -d}, {1, 0, 0}, {1, 1}},
				{{w, h, d}, {1, 0, 0}, {0, 1}},
				// Left (-X)
				{{-w, -h, -d}, {-1, 0, 0}, {0, 0}},
				{{-w, -h, d}, {-1, 0, 0}, {1, 0}},
				{{-w, h, d}, {-1, 0, 0}, {1, 1}},
				{{-w, h, -d}, {-1, 0, 0}, {0, 1}},
			};

			MeshData raw;
			raw.materialIndex = 0;
			for (const auto& v : verts)
			{
				raw.vertices.insert(raw.vertices.end(), {v.p.x, v.p.y, v.p.z});
				raw.normals.insert(raw.normals.end(), {v.n.x, v.n.y, v.n.z});
				raw.texcoords.insert(raw.texcoords.end(), {v.t.x, v.t.y});
			}
			for (uint32_t face = 0; face < 6; ++face)
			{
				uint32_t b = face * 4;
				raw.indices.insert(raw.indices.end(), {b, b + 1, b + 2, b + 2, b + 3, b});
			}
			raw.MinBounds = -dimensions * 0.5f;
			raw.MaxBounds = dimensions * 0.5f;
			return raw;
		}

		// UV-sphere with positions == normals*radius (unit normals), matching GenerateSphere.
		MeshData BuildSphere(float radius, int slices, int stacks)
		{
			MeshData raw;
			raw.materialIndex = 0;

			for (int stackIndex = 0; stackIndex <= stacks; ++stackIndex)
			{
				float stackFraction = (float)stackIndex / (float)stacks;
				float polarAngle = stackFraction * glm::pi<float>();

				for (int sliceIndex = 0; sliceIndex <= slices; ++sliceIndex)
				{
					float sliceFraction = (float)sliceIndex / (float)slices;
					float azimuthAngle = sliceFraction * 2.0f * glm::pi<float>();

					float nx = std::cos(azimuthAngle) * std::sin(polarAngle);
					float ny = std::cos(polarAngle);
					float nz = std::sin(azimuthAngle) * std::sin(polarAngle);

					raw.vertices.insert(raw.vertices.end(), {nx * radius, ny * radius, nz * radius});
					raw.texcoords.insert(raw.texcoords.end(), {sliceFraction, 1.0f - stackFraction});
					raw.normals.insert(raw.normals.end(), {nx, ny, nz});
				}
			}

			for (int stackIndex = 0; stackIndex < stacks; ++stackIndex)
			{
				for (int sliceIndex = 0; sliceIndex < slices; ++sliceIndex)
				{
					uint32_t row0 = stackIndex * (slices + 1) + sliceIndex;
					uint32_t row1 = (stackIndex + 1) * (slices + 1) + sliceIndex;
					raw.indices.insert(raw.indices.end(), {row1, row1 + 1, row0 + 1, row1, row0 + 1, row0});
				}
			}

			raw.MinBounds = {-radius, -radius, -radius};
			raw.MaxBounds = {radius, radius, radius};
			return raw;
		}

		// Flat XZ plane (up = +Y), spanning dimensions.x by dimensions.z (or dimensions.y).
		MeshData BuildPlane(const glm::vec3& dimensions)
		{
			float hx = dimensions.x * 0.5f;
			float hz = (dimensions.z != 1.0f || dimensions.y == 1.0f) ? dimensions.z * 0.5f : dimensions.y * 0.5f;
			if (dimensions.z == 1.0f && dimensions.y != 1.0f)
			{
				hz = dimensions.y * 0.5f;
			}

			MeshData raw;
			raw.materialIndex = 0;
			const glm::vec3 pos[] = {{-hx, 0.0f, -hz}, {hx, 0.0f, -hz}, {hx, 0.0f, hz}, {-hx, 0.0f, hz}};
			const glm::vec2 uv[] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};
			for (int i = 0; i < 4; ++i)
			{
				raw.vertices.insert(raw.vertices.end(), {pos[i].x, pos[i].y, pos[i].z});
				raw.normals.insert(raw.normals.end(), {0.0f, 1.0f, 0.0f});
				raw.texcoords.insert(raw.texcoords.end(), {uv[i].x, uv[i].y});
			}
			// Double-sided quad so planes are visible from both top and bottom
			raw.indices = {0, 3, 2, 0, 2, 1, 0, 2, 3, 0, 1, 2};
			raw.MinBounds = {-hx, -0.01f, -hz};
			raw.MaxBounds = {hx, 0.01f, hz};
			return raw;
		}

		// Capsule: cylinder body + two hemispheres. Normals are the unit sphere directions
		// (a good approximation; the cylinder band gets radial normals from the polar sweep).
		MeshData BuildCapsule(float radius, float height, int slices, int stacks)
		{
			MeshData raw;
			raw.materialIndex = 0;

			float cylinderHeight = std::max(0.0f, height - 2.0f * radius);
			float halfCylinderHeight = cylinderHeight * 0.5f;

			for (int stackIndex = 0; stackIndex <= stacks; ++stackIndex)
			{
				float stackFraction = (float)stackIndex / (float)stacks;
				float polarAngle = stackFraction * glm::pi<float>();
				float yOffset = (stackFraction < 0.5f) ? halfCylinderHeight : -halfCylinderHeight;

				for (int sliceIndex = 0; sliceIndex <= slices; ++sliceIndex)
				{
					float sliceFraction = (float)sliceIndex / (float)slices;
					float azimuthAngle = sliceFraction * 2.0f * glm::pi<float>();

					float nx = std::cos(azimuthAngle) * std::sin(polarAngle);
					float ny = std::cos(polarAngle);
					float nz = std::sin(azimuthAngle) * std::sin(polarAngle);

					raw.vertices.insert(raw.vertices.end(), {nx * radius, ny * radius + yOffset, nz * radius});
					raw.texcoords.insert(raw.texcoords.end(), {sliceFraction, 1.0f - stackFraction});
					raw.normals.insert(raw.normals.end(), {nx, ny, nz});
				}
			}

			for (int stackIndex = 0; stackIndex < stacks; ++stackIndex)
			{
				for (int sliceIndex = 0; sliceIndex < slices; ++sliceIndex)
				{
					uint32_t row0 = stackIndex * (slices + 1) + sliceIndex;
					uint32_t row1 = (stackIndex + 1) * (slices + 1) + sliceIndex;
					raw.indices.insert(raw.indices.end(), {row1, row1 + 1, row0 + 1, row1, row0 + 1, row0});
				}
			}

			float halfH = halfCylinderHeight + radius;
			raw.MinBounds = {-radius, -halfH, -radius};
			raw.MaxBounds = {radius, halfH, radius};
			return raw;
		}

		// Cylinder aligned to Y, base at Y=0, top at Y=height.
		// `radiusBottom`/`radiusTop` differ for a cone (top == 0) or a truncated cone.
		// Side wall + two caps. `slices` = radial segments.
		MeshData BuildConeLike(float radiusBottom, float radiusTop, float height, int slices)
		{
			MeshData raw;
			raw.materialIndex = 0;
			slices = std::max(3, slices);

			// Base at Y=0, tip/top at Y=height
			float slant = std::sqrt(height * height + (radiusBottom - radiusTop) * (radiusBottom - radiusTop));
			float ny = (slant > 0.0f) ? (radiusBottom - radiusTop) / slant : 0.0f;
			float nradial = (slant > 0.0f) ? height / slant : 1.0f;

			auto pushVert = [&](const glm::vec3& p, const glm::vec3& n, const glm::vec2& t) {
				raw.vertices.insert(raw.vertices.end(), {p.x, p.y, p.z});
				raw.normals.insert(raw.normals.end(), {n.x, n.y, n.z});
				raw.texcoords.insert(raw.texcoords.end(), {t.x, t.y});
			};

			// --- Side wall (2 rings, welded per-slice so normals are radial) ---
			uint32_t sideStart = 0;
			for (int i = 0; i <= slices; ++i)
			{
				float f = (float)i / (float)slices;
				float a = f * 2.0f * glm::pi<float>();
				float cx = std::cos(a), cz = std::sin(a);
				glm::vec3 n = glm::normalize(glm::vec3(cx * nradial, ny, cz * nradial));
				// bottom ring at Y=0
				pushVert({cx * radiusBottom, 0.0f, cz * radiusBottom}, n, {f, 0.0f});
				// top ring at Y=height
				pushVert({cx * radiusTop, height, cz * radiusTop}, n, {f, 1.0f});
			}
			for (int i = 0; i < slices; ++i)
			{
				uint32_t b = sideStart + i * 2;
				// quad (b bottom, b+1 top, b+2 next bottom, b+3 next top)
				raw.indices.insert(raw.indices.end(), {b, b + 2, b + 3, b, b + 3, b + 1});
			}

			// --- Bottom cap (facing -Y) at Y=0 ---
			if (radiusBottom > 0.0f)
			{
				uint32_t center = (uint32_t)(raw.vertices.size() / 3);
				pushVert({0.0f, 0.0f, 0.0f}, {0, -1, 0}, {0.5f, 0.5f});
				uint32_t ringStart = (uint32_t)(raw.vertices.size() / 3);
				for (int i = 0; i <= slices; ++i)
				{
					float f = (float)i / (float)slices;
					float a = f * 2.0f * glm::pi<float>();
					float cx = std::cos(a), cz = std::sin(a);
					pushVert({cx * radiusBottom, 0.0f, cz * radiusBottom}, {0, -1, 0},
							 {cx * 0.5f + 0.5f, cz * 0.5f + 0.5f});
				}
				for (int i = 0; i < slices; ++i)
				{
					raw.indices.insert(raw.indices.end(), {center, ringStart + i + 1, ringStart + i});
				}
			}

			// --- Top cap (facing +Y) at Y=height ---
			if (radiusTop > 0.0f)
			{
				uint32_t center = (uint32_t)(raw.vertices.size() / 3);
				pushVert({0.0f, height, 0.0f}, {0, 1, 0}, {0.5f, 0.5f});
				uint32_t ringStart = (uint32_t)(raw.vertices.size() / 3);
				for (int i = 0; i <= slices; ++i)
				{
					float f = (float)i / (float)slices;
					float a = f * 2.0f * glm::pi<float>();
					float cx = std::cos(a), cz = std::sin(a);
					pushVert({cx * radiusTop, height, cz * radiusTop}, {0, 1, 0}, {cx * 0.5f + 0.5f, cz * 0.5f + 0.5f});
				}
				for (int i = 0; i < slices; ++i)
				{
					raw.indices.insert(raw.indices.end(), {center, ringStart + i, ringStart + i + 1});
				}
			}

			float maxR = std::max(radiusBottom, radiusTop);
			raw.MinBounds = {-maxR, 0.0f, -maxR};
			raw.MaxBounds = {maxR, height, maxR};
			return raw;
		}

		// Upper half of a sphere (dome) + a flat bottom cap. `stacks` covers the dome only.
		MeshData BuildHemisphere(float radius, int slices, int stacks)
		{
			MeshData raw;
			raw.materialIndex = 0;
			slices = std::max(3, slices);
			stacks = std::max(2, stacks);

			auto pushVert = [&](const glm::vec3& p, const glm::vec3& n, const glm::vec2& t) {
				raw.vertices.insert(raw.vertices.end(), {p.x, p.y, p.z});
				raw.normals.insert(raw.normals.end(), {n.x, n.y, n.z});
				raw.texcoords.insert(raw.texcoords.end(), {t.x, t.y});
			};

			// Dome: polar angle 0 (top) .. pi/2 (equator)
			for (int st = 0; st <= stacks; ++st)
			{
				float sf = (float)st / (float)stacks;
				float polar = sf * glm::half_pi<float>();
				for (int sl = 0; sl <= slices; ++sl)
				{
					float slf = (float)sl / (float)slices;
					float az = slf * 2.0f * glm::pi<float>();
					float nx = std::cos(az) * std::sin(polar);
					float ny = std::cos(polar);
					float nz = std::sin(az) * std::sin(polar);
					pushVert({nx * radius, ny * radius, nz * radius}, {nx, ny, nz}, {slf, 1.0f - sf});
				}
			}
			for (int st = 0; st < stacks; ++st)
			{
				for (int sl = 0; sl < slices; ++sl)
				{
					uint32_t row0 = st * (slices + 1) + sl;
					uint32_t row1 = (st + 1) * (slices + 1) + sl;
					raw.indices.insert(raw.indices.end(), {row1, row1 + 1, row0 + 1, row1, row0 + 1, row0});
				}
			}

			// Flat bottom cap at y = 0, facing -Y
			uint32_t center = (uint32_t)(raw.vertices.size() / 3);
			pushVert({0.0f, 0.0f, 0.0f}, {0, -1, 0}, {0.5f, 0.5f});
			uint32_t ringStart = (uint32_t)(raw.vertices.size() / 3);
			for (int sl = 0; sl <= slices; ++sl)
			{
				float slf = (float)sl / (float)slices;
				float az = slf * 2.0f * glm::pi<float>();
				float cx = std::cos(az), cz = std::sin(az);
				pushVert({cx * radius, 0.0f, cz * radius}, {0, -1, 0}, {cx * 0.5f + 0.5f, cz * 0.5f + 0.5f});
			}
			for (int sl = 0; sl < slices; ++sl)
			{
				raw.indices.insert(raw.indices.end(), {center, ringStart + sl + 1, ringStart + sl});
			}

			raw.MinBounds = {-radius, 0.0f, -radius};
			raw.MaxBounds = {radius, radius, radius};
			return raw;
		}

		// Torus in the XZ plane. `majorRadius` = ring center distance, `minorRadius` = tube radius.
		// `slices` = segments around the ring, `stacks` = segments around the tube.
		MeshData BuildTorus(float majorRadius, float minorRadius, int slices, int stacks)
		{
			MeshData raw;
			raw.materialIndex = 0;
			slices = std::max(3, slices);
			stacks = std::max(3, stacks);

			for (int i = 0; i <= slices; ++i)
			{
				float u = (float)i / (float)slices * 2.0f * glm::pi<float>();
				float cu = std::cos(u), su = std::sin(u);
				for (int j = 0; j <= stacks; ++j)
				{
					float v = (float)j / (float)stacks * 2.0f * glm::pi<float>();
					float cv = std::cos(v), sv = std::sin(v);

					// Tube center for this ring position, in XZ plane
					glm::vec3 center = {majorRadius * cu, 0.0f, majorRadius * su};
					glm::vec3 normal = {cv * cu, sv, cv * su};
					glm::vec3 pos = center + minorRadius * normal;

					raw.vertices.insert(raw.vertices.end(), {pos.x, pos.y, pos.z});
					raw.normals.insert(raw.normals.end(), {normal.x, normal.y, normal.z});
					raw.texcoords.insert(raw.texcoords.end(), {(float)i / (float)slices, (float)j / (float)stacks});
				}
			}
			int stride = stacks + 1;
			for (int i = 0; i < slices; ++i)
			{
				for (int j = 0; j < stacks; ++j)
				{
					uint32_t a = i * stride + j;
					uint32_t b = (i + 1) * stride + j;
					raw.indices.insert(raw.indices.end(), {a, a + 1, b + 1, a, b + 1, b});
				}
			}

			float outer = majorRadius + minorRadius;
			raw.MinBounds = {-outer, -minorRadius, -outer};
			raw.MaxBounds = {outer, minorRadius, outer};
			return raw;
		}

		// Trefoil knot: a tube of radius `minorRadius` swept along a (2,3) torus knot curve
		// scaled by `majorRadius`. `slices` = segments along the curve, `stacks` = segments
		// around the tube. The frame is built from the analytic tangent plus an arbitrary
		// reference axis, which is stable here because the curve never runs parallel to +Y.
		MeshData BuildKnot(float majorRadius, float minorRadius, int slices, int stacks)
		{
			MeshData raw;
			raw.materialIndex = 0;
			slices = std::max(16, slices);
			stacks = std::max(3, stacks);

			// Curve point and tangent for the standard trefoil parametrization.
			auto curve = [](float t) {
				return glm::vec3(std::sin(t) + 2.0f * std::sin(2.0f * t), std::cos(t) - 2.0f * std::cos(2.0f * t),
								 -std::sin(3.0f * t));
			};
			auto tangent = [](float t) {
				return glm::normalize(glm::vec3(std::cos(t) + 4.0f * std::cos(2.0f * t),
												-std::sin(t) + 4.0f * std::sin(2.0f * t), -3.0f * std::cos(3.0f * t)));
			};

			// The raw trefoil spans roughly [-3, 3]; normalize so majorRadius is the outer extent.
			const float curveScale = majorRadius / 3.0f;

			glm::vec3 minB(std::numeric_limits<float>::max());
			glm::vec3 maxB(std::numeric_limits<float>::lowest());

			for (int i = 0; i <= slices; ++i)
			{
				float t = (float)i / (float)slices * 2.0f * glm::pi<float>();
				glm::vec3 center = curve(t) * curveScale;
				glm::vec3 tan = tangent(t);

				// Build an orthonormal frame around the tangent.
				glm::vec3 ref = std::abs(tan.y) < 0.9f ? glm::vec3(0, 1, 0) : glm::vec3(1, 0, 0);
				glm::vec3 normal = glm::normalize(glm::cross(tan, ref));
				glm::vec3 binormal = glm::cross(tan, normal);

				for (int j = 0; j <= stacks; ++j)
				{
					float v = (float)j / (float)stacks * 2.0f * glm::pi<float>();
					glm::vec3 n = glm::normalize(std::cos(v) * normal + std::sin(v) * binormal);
					glm::vec3 pos = center + minorRadius * n;

					raw.vertices.insert(raw.vertices.end(), {pos.x, pos.y, pos.z});
					raw.normals.insert(raw.normals.end(), {n.x, n.y, n.z});
					raw.texcoords.insert(raw.texcoords.end(), {(float)i / (float)slices, (float)j / (float)stacks});

					minB = glm::min(minB, pos);
					maxB = glm::max(maxB, pos);
				}
			}

			int stride = stacks + 1;
			for (int i = 0; i < slices; ++i)
			{
				for (int j = 0; j < stacks; ++j)
				{
					uint32_t a = i * stride + j;
					uint32_t b = (i + 1) * stride + j;
					raw.indices.insert(raw.indices.end(), {a, a + 1, b + 1, a, b + 1, b});
				}
			}

			raw.MinBounds = minB;
			raw.MaxBounds = maxB;
			return raw;
		}
	} // namespace

	PendingModelData GeometryGenerator::GeneratePrimitivePendingData(const std::string& type,
																	 const ProceduralParameters& params)
	{
		PendingModelData data;

		MeshData raw;
		bool generated = true;

		if (type == ":cube:")
		{
			raw = BuildCube(params.Dimensions);
		}
		else if (type == ":sphere:")
		{
			raw = BuildSphere(params.Radius, params.Slices, params.Stacks);
		}
		else if (type == ":plane:")
		{
			raw = BuildPlane(params.Dimensions);
		}
		else if (type == ":cylinder:")
		{
			raw = BuildConeLike(params.Radius, params.Radius, params.Height, params.Slices);
		}
		else if (type == ":cone:")
		{
			raw = BuildConeLike(params.Radius, 0.0f, params.Height, params.Slices);
		}
		else if (type == ":hemisphere:")
		{
			raw = BuildHemisphere(params.Radius, params.Slices, params.Stacks);
		}
		else if (type == ":torus:")
		{
			// The tube must stay thinner than the ring it wraps, otherwise the torus turns
			// inside out and renders as a shapeless blob.
			float tube = std::min(params.InnerRadius, params.Radius * 0.95f);
			raw = BuildTorus(params.Radius, tube, params.Slices, params.Stacks);
		}
		else if (type == ":knot:")
		{
			float tube = std::min(params.InnerRadius, params.Radius * 0.5f);
			raw = BuildKnot(params.Radius, tube, params.Slices, params.Stacks);
		}
		else
		{
			generated = false;
		}

		if (!generated)
		{
			return data; // isValid stays false → OnLoaded is a no-op
		}

		data.meshes.push_back(std::move(raw));
		data.instances.push_back(MeshInstance{0, glm::mat4(1.0f)});

		MaterialData defaultMat;
		// Named so it shows up as a selectable entry in the Material Editor's list.
		defaultMat.name = "Primitive Material";
		defaultMat.albedoColor = {1.0f, 1.0f, 1.0f, 1.0f};
		defaultMat.roughness = 0.5f;
		defaultMat.metalness = 0.0f;
		data.materials.push_back(defaultMat);

		data.isValid = true;
		return data;
	}
} // namespace Chained
