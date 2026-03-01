#ifndef CH_LAMBDA_COMMAND_H
#define CH_LAMBDA_COMMAND_H

#include "editor_command.h"
#include <functional>
#include <string>

namespace CHEngine
{

class LambdaCommand : public IEditorCommand
{
public:
    using ActionFn = std::function<void()>;

    LambdaCommand(const std::string& name, ActionFn execute, ActionFn undo)
        : m_Name(name),
          m_Execute(execute),
          m_Undo(undo)
    {
    }

    void Execute() override
    {
        if (m_Execute)
        {
            m_Execute();
        }
    }
    void Undo() override
    {
        if (m_Undo)
        {
            m_Undo();
        }
    }
    std::string GetName() const override
    {
        return m_Name;
    }

private:
    std::string m_Name;
    ActionFn m_Execute;
    ActionFn m_Undo;
};

} // namespace CHEngine

#endif // CH_LAMBDA_COMMAND_H
