#ifndef CH_IMGUI_INTERFACE_H
#define CH_IMGUI_INTERFACE_H

#include "engine/core/layer.h"

namespace Chained
{
    class CH_API IImGuiLayer : public Layer
    {
    public:
        IImGuiLayer(const std::string& name = "ImGuiLayer") : Layer(name) {}
        virtual ~IImGuiLayer() = default;

        virtual void Begin() = 0;
        virtual void End() = 0;
        virtual void BlockEvents(bool block) = 0;
        virtual bool RefreshFontAtlasTexture() = 0;

        virtual void* GetContext() const = 0;
        virtual void* AddFontFromFile(const std::string& path, float size, const void* config = nullptr, const void* ranges = nullptr) = 0;
    };
}

#endif
