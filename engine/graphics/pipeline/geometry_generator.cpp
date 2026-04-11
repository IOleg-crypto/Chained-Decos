#include "geometry_generator.h"
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

namespace CHEngine
{
    Mesh GeometryGenerator::GenerateUnitCube()
    {
        float vertices[] = {
            -0.5f,  0.5f, -0.5f,
            -0.5f, -0.5f, -0.5f,
             0.5f, -0.5f, -0.5f,
             0.5f, -0.5f, -0.5f,
             0.5f,  0.5f, -0.5f,
            -0.5f,  0.5f, -0.5f,

            -0.5f, -0.5f,  0.5f,
            -0.5f, -0.5f, -0.5f,
            -0.5f,  0.5f, -0.5f,
            -0.5f,  0.5f,  0.5f,
            -0.5f,  0.5f,  0.5f,
            -0.5f, -0.5f,  0.5f,

             0.5f, -0.5f, -0.5f,
             0.5f, -0.5f,  0.5f,
             0.5f,  0.5f,  0.5f,
             0.5f,  0.5f,  0.5f,
             0.5f,  0.5f, -0.5f,
             0.5f, -0.5f, -0.5f,

            -0.5f, -0.5f,  0.5f,
            -0.5f,  0.5f,  0.5f,
             0.5f,  0.5f,  0.5f,
             0.5f,  0.5f,  0.5f,
             0.5f, -0.5f,  0.5f,
            -0.5f, -0.5f,  0.5f,

            -0.5f,  0.5f, -0.5f,
             0.5f,  0.5f, -0.5f,
             0.5f,  0.5f,  0.5f,
             0.5f,  0.5f,  0.5f,
            -0.5f,  0.5f,  0.5f,
            -0.5f,  0.5f, -0.5f,

            -0.5f, -0.5f, -0.5f,
            -0.5f, -0.5f,  0.5f,
             0.5f, -0.5f, -0.5f,
             0.5f, -0.5f, -0.5f,
            -0.5f, -0.5f,  0.5f,
             0.5f, -0.5f,  0.5f
        };

        auto vbo = VertexBuffer::Create(vertices, sizeof(vertices));
        vbo->SetLayout({{ShaderDataType::Float3, "a_Position"}});
        auto vao = VertexArray::Create();
        vao->AddVertexBuffer(vbo);
        
        uint32_t indices[36];
        for (uint32_t index = 0; index < 36; ++index) indices[index] = index;
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
            -0.5f, -0.5f, -0.5f,  // 0: left  bottom back
             0.5f, -0.5f, -0.5f,  // 1: right bottom back
             0.5f,  0.5f, -0.5f,  // 2: right top    back
            -0.5f,  0.5f, -0.5f,  // 3: left  top    back
            -0.5f, -0.5f,  0.5f,  // 4: left  bottom front
             0.5f, -0.5f,  0.5f,  // 5: right bottom front
             0.5f,  0.5f,  0.5f,  // 6: right top    front
            -0.5f,  0.5f,  0.5f   // 7: left  top    front
        };

        // 12 edges × 2 indices = 24 indices for GL_LINES
        uint32_t indices[] = {
            // Back face edges
            0, 1,  1, 2,  2, 3,  3, 0,
            // Front face edges
            4, 5,  5, 6,  6, 7,  7, 4,
            // Connecting edges
            0, 4,  1, 5,  2, 6,  3, 7
        };

        auto vbo = VertexBuffer::Create(vertices, sizeof(vertices));
        vbo->SetLayout({{ShaderDataType::Float3, "a_Position"}});
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

        for (int stackIndex = 0; stackIndex <= stacks; ++stackIndex) {
            float stackFraction = (float)stackIndex / (float)stacks;
            float polarAngle = stackFraction * glm::pi<float>();

            for (int sliceIndex = 0; sliceIndex <= slices; ++sliceIndex) {
                float sliceFraction = (float)sliceIndex / (float)slices;
                float azimuthAngle = sliceFraction * 2.0f * glm::pi<float>();

                float positionX = std::cos(azimuthAngle) * std::sin(polarAngle);
                float positionY = std::cos(polarAngle);
                float positionZ = std::sin(azimuthAngle) * std::sin(polarAngle);

                vertices.push_back(positionX * radius);
                vertices.push_back(positionY * radius);
                vertices.push_back(positionZ * radius);
            }
        }

        for (int stackIndex = 0; stackIndex < stacks; ++stackIndex) {
            for (int sliceIndex = 0; sliceIndex < slices; ++sliceIndex) {
                indices.push_back((stackIndex + 1) * (slices + 1) + sliceIndex);
                indices.push_back(stackIndex * (slices + 1) + sliceIndex);
                indices.push_back(stackIndex * (slices + 1) + sliceIndex + 1);
                indices.push_back((stackIndex + 1) * (slices + 1) + sliceIndex);
                indices.push_back(stackIndex * (slices + 1) + sliceIndex + 1);
                indices.push_back((stackIndex + 1) * (slices + 1) + (sliceIndex + 1));
            }
        }

        auto vbo = VertexBuffer::Create(vertices.data(), (uint32_t)vertices.size() * sizeof(float));
        vbo->SetLayout({{ShaderDataType::Float3, "a_Position"}});
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

    Mesh GeometryGenerator::GenerateGrid(int slices, float spacing)
    {
        std::vector<float> vertices;
        for (int lineIndex = -slices; lineIndex <= slices; ++lineIndex)
        {
            vertices.push_back((float)lineIndex * spacing); vertices.push_back(0); vertices.push_back((float)-slices * spacing);
            vertices.push_back((float)lineIndex * spacing); vertices.push_back(0); vertices.push_back((float)slices * spacing);

            vertices.push_back((float)-slices * spacing); vertices.push_back(0); vertices.push_back((float)lineIndex * spacing);
            vertices.push_back((float)slices * spacing); vertices.push_back(0); vertices.push_back((float)lineIndex * spacing);
        }

        auto vbo = VertexBuffer::Create(vertices.data(), (uint32_t)vertices.size() * sizeof(float));
        vbo->SetLayout({{ShaderDataType::Float3, "a_Position"}});
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
        float vertices[] = {
            -size, 0.0f, -size,
             size, 0.0f, -size,
             size, 0.0f,  size,
            -size, 0.0f,  size
        };

        auto vbo = VertexBuffer::Create(vertices, sizeof(vertices));
        vbo->SetLayout({{ShaderDataType::Float3, "a_Position"}});
        auto vao = VertexArray::Create();
        vao->AddVertexBuffer(vbo);

        Mesh mesh;
        mesh.VAO = vao;
        mesh.VertexCount = 4;
        mesh.TriangleCount = 2;
        return mesh;
    }

    Mesh GeometryGenerator::GenerateCube(const glm::vec3 &dimensions)
    {
        float halfWidth = dimensions.x * 0.5f;
        float halfHeight = dimensions.y * 0.5f;
        float halfDepth = dimensions.z * 0.5f;

        float vertices[] = {
            -halfWidth,-halfHeight, halfDepth,  halfWidth,-halfHeight, halfDepth,  halfWidth, halfHeight, halfDepth, -halfWidth, halfHeight, halfDepth,
            -halfWidth,-halfHeight,-halfDepth, -halfWidth, halfHeight,-halfDepth,  halfWidth, halfHeight,-halfDepth,  halfWidth,-halfHeight,-halfDepth,
            -halfWidth, halfHeight,-halfDepth, -halfWidth, halfHeight, halfDepth,  halfWidth, halfHeight, halfDepth,  halfWidth, halfHeight,-halfDepth,
            -halfWidth,-halfHeight,-halfDepth,  halfWidth,-halfHeight,-halfDepth,  halfWidth,-halfHeight, halfDepth, -halfWidth,-halfHeight, halfDepth,
             halfWidth,-halfHeight,-halfDepth,  halfWidth, halfHeight,-halfDepth,  halfWidth, halfHeight, halfDepth,  halfWidth,-halfHeight, halfDepth,
            -halfWidth,-halfHeight,-halfDepth, -halfWidth,-halfHeight, halfDepth, -halfWidth, halfHeight, halfDepth, -halfWidth, halfHeight,-halfDepth
        };

        auto vbo = VertexBuffer::Create(vertices, sizeof(vertices));
        vbo->SetLayout({{ShaderDataType::Float3, "a_Position"}});
        auto vao = VertexArray::Create();
        vao->AddVertexBuffer(vbo);
        
        uint32_t indices[36];
        for (uint32_t index = 0; index < 36; ++index) indices[index] = index;
        auto ebo = IndexBuffer::Create(indices, 36);
        vao->SetIndexBuffer(ebo);

        Mesh mesh;
        mesh.VAO = vao;
        mesh.VertexCount = 36;
        mesh.TriangleCount = 12;
        return mesh;
    }
}
