#ifndef OBJECT_FACTORY_H
#define OBJECT_FACTORY_H

#include "../ToolManager/IToolManager.h"
#include <string>


class Editor;

// ObjectFactory - handles creation of map objects
class ObjectFactory
{
public:
    ObjectFactory(Editor *editor);
    ~ObjectFactory() = default;

    // Create object based on active tool and selected model name
    void CreateObject(::Tool activeTool, const std::string &selectedModelName);

private:
    Editor *m_editor;
};

#endif // OBJECT_FACTORY_H
