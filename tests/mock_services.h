#pragma once

#include <gmock/gmock.h>
#include "engine/assets/asset_loader.h"

namespace Chained {

class MockAssetLoader : public IAssetLoader {
public:
    MOCK_METHOD(std::shared_ptr<Asset>, Create, (), (const, override));
    MOCK_METHOD(bool, Load, (std::shared_ptr<Asset> asset, const LoadContext& ctx, std::string* outError), (override));
    MOCK_METHOD(bool, IsAsync, (), (const, override));
};

} // namespace Chained
