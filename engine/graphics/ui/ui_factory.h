#ifndef CH_UI_FACTORY_H
#define CH_UI_FACTORY_H

#include "engine/scene/entity.h"
#include <functional>
#include <string>
#include <unordered_map>

namespace Chained
{
using UIBuilderFunc = std::function<void(Entity)>;

class UIFactory
{
public:
    // Registers a new UI type with its building logic.
    static void Register(const std::string& type, UIBuilderFunc builder);

    // Creates components for the given entity based on the type string.
    // Returns true if the type was found and processed.
    static bool Create(const std::string& type, Entity entity);

    // Initializes the factory with all standard engine UI components.
    static void Initialize();

private:
    static std::unordered_map<std::string, UIBuilderFunc> s_Builders;
};
} // namespace Chained

#endif // CH_UI_FACTORY_H
