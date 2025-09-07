#include "ModelConfig.h"

float LoadingStats::GetSuccessRate() const
{
    return totalModels > 0 ? (float)loadedModels / totalModels : 0.0f;
}
