#ifndef IRENDER_COMMAND_H
#define IRENDER_COMMAND_H

// Base interface for all render commands
// Commands encapsulate rendering operations without exposing dependencies
class IRenderCommand
{
public:
    virtual ~IRenderCommand() = default;

    // Execute the rendering command
    virtual void Execute() = 0;

    // Optional: Get command type for debugging
    virtual const char *GetCommandType() const = 0;
};

#endif // IRENDER_COMMAND_H




