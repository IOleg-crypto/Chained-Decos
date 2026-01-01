#include "SceneMetadata.h"
#include <iomanip>
#include <sstream>


namespace CHEngine
{

std::string SceneMetadata::GetRelativeTimestamp() const
{
    return SceneMetadataReader::CalculateRelativeTime(modified);
}

SceneMetadata SceneMetadataReader::LoadFromFile(const std::string &scenePath)
{
    SceneMetadata metadata;
    metadata.path = scenePath;

    // Extract name from path
    size_t lastSlash = scenePath.find_last_of("/\\");
    size_t lastDot = scenePath.find_last_of(".");
    if (lastSlash != std::string::npos && lastDot != std::string::npos)
    {
        metadata.name = scenePath.substr(lastSlash + 1, lastDot - lastSlash - 1);
    }

    // TODO: Parse JSON metadata from .chscene file header
    // For now, use default values
    metadata.type = SceneType::Game;
    metadata.created = std::chrono::system_clock::now();
    metadata.modified = std::chrono::system_clock::now();

    return metadata;
}

bool SceneMetadataReader::SaveToFile(const std::string &scenePath, const SceneMetadata &metadata)
{
    // TODO: Write metadata to .chscene file header as JSON
    return true;
}

std::string
SceneMetadataReader::CalculateRelativeTime(const std::chrono::system_clock::time_point &timestamp)
{
    auto now = std::chrono::system_clock::now();
    auto diff = std::chrono::duration_cast<std::chrono::seconds>(now - timestamp);

    auto seconds = diff.count();

    if (seconds < 60)
        return std::to_string(seconds) + " sec ago";
    else if (seconds < 3600)
        return std::to_string(seconds / 60) + " min ago";
    else if (seconds < 86400)
        return std::to_string(seconds / 3600) + " hrs ago";
    else
        return std::to_string(seconds / 86400) + " days ago";
}

} // namespace CHEngine
