#ifndef CORE_OBJECT_H
#define CORE_OBJECT_H

#include "core/macros.h"
#include <string>

// Base class for all engine objects (like Godot's Object)
// Provides common functionality: naming, type info, etc.
class Object
{
public:
    Object() = default;
    virtual ~Object() = default;

    // Type information
    virtual const char *GetClassName() const
    {
        return "Object";
    }

    // Name property
    const std::string &GetName() const
    {
        return m_Name;
    }
    void SetName(const std::string &name)
    {
        m_Name = name;
    }

private:
    std::string m_Name;
};

#endif // CORE_OBJECT_H
