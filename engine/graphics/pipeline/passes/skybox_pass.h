#pragma once

#include "engine/graphics/pipeline/render_pass.h"

namespace Chained {

    class SkyboxPass : public IRenderPass
    {
    public:
        void Execute(const RenderContext& ctx) override;
        const std::string& GetName() const override { static std::string name = "SkyboxPass"; return name; }
    };

}
