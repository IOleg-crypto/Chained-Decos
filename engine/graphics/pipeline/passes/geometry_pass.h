#ifndef GEOMETRY_PASS_H
#define GEOMETRY_PASS_H

#include "engine/graphics/pipeline/render_pass.h"

namespace Chained {

    class GeometryPass : public IRenderPass
    {
    public:
        void Execute(const RenderContext& ctx) override;
        const std::string& GetName() const override { static std::string name = "GeometryPass"; return name; }
    };

}
#endif
