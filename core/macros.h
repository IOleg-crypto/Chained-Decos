#ifndef CORE_MACROS_H
#define CORE_MACROS_H

// Class registration macro (like GDCLASS in Godot)
#define REGISTER_CLASS(ClassName, BaseClass)                                                       \
public:                                                                                            \
    using Super = BaseClass;                                                                       \
    static const char *GetStaticClassName()                                                        \
    {                                                                                              \
        return #ClassName;                                                                         \
    }                                                                                              \
    virtual const char *GetClassName() const override                                              \
    {                                                                                              \
        return #ClassName;                                                                         \
    }

// Property generation macros
#define PROPERTY(type, name)                                                                       \
private:                                                                                           \
    type m_##name;                                                                                 \
                                                                                                   \
public:                                                                                            \
    const type &Get##name() const                                                                  \
    {                                                                                              \
        return m_##name;                                                                           \
    }                                                                                              \
    void Set##name(const type &value)                                                              \
    {                                                                                              \
        m_##name = value;                                                                          \
    }

#define PROPERTY_REF(type, name)                                                                   \
private:                                                                                           \
    type m_##name;                                                                                 \
                                                                                                   \
public:                                                                                            \
    type &Get##name()                                                                              \
    {                                                                                              \
        return m_##name;                                                                           \
    }                                                                                              \
    const type &Get##name() const                                                                  \
    {                                                                                              \
        return m_##name;                                                                           \
    }

// Disable copy/move macros
#define DISABLE_COPY(ClassName)                                                                    \
    ClassName(const ClassName &) = delete;                                                         \
    ClassName &operator=(const ClassName &) = delete;

#define DISABLE_MOVE(ClassName)                                                                    \
    ClassName(ClassName &&) = delete;                                                              \
    ClassName &operator=(ClassName &&) = delete;

#define DISABLE_COPY_AND_MOVE(ClassName)                                                           \
    DISABLE_COPY(ClassName)                                                                        \
    DISABLE_MOVE(ClassName)

#endif // CORE_MACROS_H
