#ifndef MODEL_ANALYZER_H
#define MODEL_ANALYZER_H

#include <string>
#include <vector>
#include "scene/resources/map/Core/MapData.h"

/**
 * @brief Utility class for analyzing map files and determining model requirements
 * 
 * ModelAnalyzer provides static methods for analyzing map files to determine
 * which 3D models are required for rendering. It supports both editor format
 * (JSON with metadata) and game format (direct JSON array) map files.
 */
class ModelAnalyzer
{
public:
    /**
     * @brief Determines the model name required for a specific map object type
     * 
     * @param objectType The type of map object (from MapObjectType enum)
     * @param modelName Optional explicit model name from the map object
     * @return Model name if required, empty string if no model needed
     * 
     * @note Handles special cases like LIGHT objects that may actually be MODEL objects
     *       due to map editor export issues
     */
    static std::string GetModelNameForObjectType(int objectType, const std::string& modelName = "");
    
    /**
     * @brief Analyzes a map file and returns list of required model names
     * 
     * @param mapIdentifier Map file path or map name (will be converted to full path)
     * @return Vector of model names required for the map (always includes player model)
     * 
     * @note Automatically detects map format (editor vs game) and uses appropriate parser
     * @note Model names are normalized (file extensions and paths removed)
     * @note Duplicates are automatically filtered out
     */
    static std::vector<std::string> GetModelsRequiredForMap(const std::string& mapIdentifier);

private:
    /**
     * @brief Analyzes editor format map (JSON with metadata)
     * 
     * @param content Full JSON content of the map file
     * @param requiredModels Output vector to append required models to
     */
    static void AnalyzeEditorFormat(const std::string& content, std::vector<std::string>& requiredModels);
    
    /**
     * @brief Analyzes game format map (direct JSON array)
     * 
     * @param content Full JSON content of the map file
     * @param requiredModels Output vector to append required models to
     */
    static void AnalyzeGameFormat(const std::string& content, std::vector<std::string>& requiredModels);
    
    /**
     * @brief Converts map identifier to full file path
     * 
     * @param mapIdentifier Map name or partial path
     * @return Full path to map file
     */
    static std::string ConvertToMapPath(const std::string& mapIdentifier);
    
    /**
     * @brief Normalizes model name by removing path and extension
     * 
     * @param modelName Raw model name from map file
     * @return Normalized model name (filename without extension)
     */
    static std::string NormalizeModelName(const std::string& modelName);
    
    /**
     * @brief Adds model to requirements list if not already present
     * 
     * @param modelName Model name to add
     * @param requiredModels Vector to add model to
     * @return true if model was added, false if already present
     */
    static bool AddModelIfUnique(const std::string& modelName, std::vector<std::string>& requiredModels);
};

#endif // MODEL_ANALYZER_H
