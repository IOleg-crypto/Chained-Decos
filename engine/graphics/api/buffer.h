#ifndef CH_BUFFER_H
#define CH_BUFFER_H

#include <string>
#include <vector>
#include <memory>
#include <initializer_list>
#include "engine/common/engine_assert.h"

namespace Chained
{

enum class VertexAttributeType
{
    None = 0, Float, Float2, Float3, Float4, Mat3, Mat4, Int, Int2, Int3, Int4, Bool
};

    constexpr uint32_t VertexAttributeTypeSize(VertexAttributeType type)
    {
        switch (type)
        {
            case VertexAttributeType::Float:    return 4;
            case VertexAttributeType::Float2:   return 4 * 2;
            case VertexAttributeType::Float3:   return 4 * 3;
            case VertexAttributeType::Float4:   return 4 * 4;
            case VertexAttributeType::Mat3:     return 4 * 3 * 3; 
            case VertexAttributeType::Mat4:     return 4 * 4 * 4;
            case VertexAttributeType::Int:      return 4;
            case VertexAttributeType::Int2:     return 4 * 2;
            case VertexAttributeType::Int3:     return 4 * 3;
            case VertexAttributeType::Int4:     return 4 * 4;
            case VertexAttributeType::Bool:     return 1;
        }

        CH_CORE_ASSERT(false, "Unknown VertexAttributeType!");
        return 0;
    }

struct BufferElement
{
    std::string Name;
    VertexAttributeType Type;
    uint32_t Size;
    size_t Offset;
    bool Normalized;
    bool Instanced;

    BufferElement() = default;

    BufferElement(VertexAttributeType type, const std::string& name, bool normalized = false, bool instanced = false)
        : Name(name), Type(type), Size(VertexAttributeTypeSize(type)), Offset(0), Normalized(normalized), Instanced(instanced)
    {
    }

    uint32_t GetComponentCount() const
    {
        switch (Type)
        {
            case VertexAttributeType::Float:   return 1;
            case VertexAttributeType::Float2:  return 2;
            case VertexAttributeType::Float3:  return 3;
            case VertexAttributeType::Float4:  return 4;
            case VertexAttributeType::Mat3:    return 3; // 3* float3
            case VertexAttributeType::Mat4:    return 4; // 4* float4
            case VertexAttributeType::Int:     return 1;
            case VertexAttributeType::Int2:    return 2;
            case VertexAttributeType::Int3:    return 3;
            case VertexAttributeType::Int4:    return 4;
            case VertexAttributeType::Bool:    return 1;
        }

        CH_CORE_ASSERT(false, "Unknown VertexAttributeType!");
        return 0;
    }
};

class BufferLayout
{
public:
    BufferLayout() {}

    BufferLayout(const std::initializer_list<BufferElement>& elements)
        : m_Elements(elements)
    {
        CalculateOffsetsAndStride();
    }

    uint32_t GetStride() const { return m_Stride; }
    const std::vector<BufferElement>& GetElements() const { return m_Elements; }

    std::vector<BufferElement>::iterator begin() { return m_Elements.begin(); }
    std::vector<BufferElement>::iterator end() { return m_Elements.end(); }
    std::vector<BufferElement>::const_iterator begin() const { return m_Elements.begin(); }
    std::vector<BufferElement>::const_iterator end() const { return m_Elements.end(); }
private:
    void CalculateOffsetsAndStride()
    {
        size_t offset = 0;
        m_Stride = 0;
        for (auto& element : m_Elements)
        {
            element.Offset = offset;
            offset += element.Size;
            m_Stride += element.Size;
        }
    }
private:
    std::vector<BufferElement> m_Elements;
    uint32_t m_Stride = 0;
};

class VertexBuffer
{
public:
    virtual ~VertexBuffer() = default;

    virtual void Bind() const = 0;
    virtual void Unbind() const = 0;

    virtual void SetData(const void* data, uint32_t size) = 0;

    virtual const BufferLayout& GetLayout() const = 0;
    virtual void SetLayout(const BufferLayout& layout) = 0;

    static std::shared_ptr<VertexBuffer> Create(uint32_t size);
    static std::shared_ptr<VertexBuffer> Create(const float* vertices, uint32_t size);
};

class IndexBuffer
{
public:
    virtual ~IndexBuffer() = default;

    virtual void Bind() const = 0;
    virtual void Unbind() const = 0;

    virtual uint32_t GetCount() const = 0;

    static std::shared_ptr<IndexBuffer> Create(const uint32_t* indices, uint32_t count);
};

class UniformBuffer
{
public:
    virtual ~UniformBuffer() = default;

    virtual void SetData(const void* data, uint32_t size, uint32_t offset = 0) = 0;

    static std::shared_ptr<UniformBuffer> Create(uint32_t size, uint32_t binding);
};

} // namespace Chained

#endif // CH_BUFFER_H
