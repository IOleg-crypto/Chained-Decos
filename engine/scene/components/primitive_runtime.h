#ifndef CH_PRIMITIVE_RUNTIME_H
#define CH_PRIMITIVE_RUNTIME_H

#include <memory>

namespace Chained
{
class ModelAsset;

/// @brief Runtime-only state for PrimitiveComponent (NOT serialized, NOT reflected).
/// Kept as a separate component so rfl::to_view on PrimitiveComponent never sees
/// non-trivial types (shared_ptr) that break reflection field-index alignment.
struct PrimitiveRuntimeState
{
    bool Dirty = true;
    std::shared_ptr<ModelAsset> Asset;
};

} // namespace Chained

#endif // CH_PRIMITIVE_RUNTIME_H
