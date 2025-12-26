#include "SceneSerializer.h"
#include "SceneLoader.h"
#include "core/Log.h"
#include <fstream>
#include <vector>


namespace CHEngine
{
struct SceneHeader
{
    char magic[4] = {'C', 'H', 'S', 'C'};
    uint32_t version = 1;
};

SceneSerializer::SceneSerializer(const std::shared_ptr<GameScene> &scene) : m_Scene(scene)
{
}

bool SceneSerializer::SerializeBinary(const std::string &filepath)
{
    std::ofstream fout(filepath, std::ios::out | std::ios::binary);
    if (!fout.is_open())
        return false;

    SceneHeader header;
    fout.write((char *)&header, sizeof(header));

    // 1. Metadata
    auto &meta = m_Scene->GetMapMetaData();

    auto writeString = [&](const std::string &str)
    {
        uint32_t size = (uint32_t)str.size();
        fout.write((char *)&size, sizeof(size));
        fout.write(str.data(), size);
    };

    writeString(meta.name);
    writeString(meta.displayName);
    writeString(meta.description);
    writeString(meta.author);
    writeString(meta.version);
    fout.write((char *)&meta.startPosition, sizeof(Vector3));
    fout.write((char *)&meta.endPosition, sizeof(Vector3));
    fout.write((char *)&meta.skyColor, sizeof(Color));
    fout.write((char *)&meta.groundColor, sizeof(Color));
    fout.write((char *)&meta.difficulty, sizeof(float));
    writeString(meta.skyboxTexture);
    fout.write((char *)&meta.sceneType, sizeof(SceneType));

    // 2. Objects
    auto &objects = m_Scene->GetMapObjects();
    uint32_t objectCount = (uint32_t)objects.size();
    fout.write((char *)&objectCount, sizeof(objectCount));

    for (const auto &obj : objects)
    {
        writeString(obj.name);
        fout.write((char *)&obj.type, sizeof(MapObjectType));
        fout.write((char *)&obj.position, sizeof(Vector3));
        fout.write((char *)&obj.rotation, sizeof(Vector3));
        fout.write((char *)&obj.scale, sizeof(Vector3));
        fout.write((char *)&obj.color, sizeof(Color));
        writeString(obj.modelName);
        fout.write((char *)&obj.radius, sizeof(float));
        fout.write((char *)&obj.height, sizeof(float));
        fout.write((char *)&obj.size, sizeof(Vector2));
        fout.write((char *)&obj.isPlatform, sizeof(bool));
        fout.write((char *)&obj.isObstacle, sizeof(bool));
        writeString(obj.scriptPath);
    }

    // 3. UI Elements
    auto &uiElements = m_Scene->GetUIElements();
    uint32_t uiCount = (uint32_t)uiElements.size();
    fout.write((char *)&uiCount, sizeof(uiCount));

    for (const auto &elem : uiElements)
    {
        writeString(elem.name);
        writeString(elem.type);
        fout.write((char *)&elem.isActive, sizeof(bool));
        fout.write((char *)&elem.anchor, sizeof(int));
        fout.write((char *)&elem.position, sizeof(Vector2));
        fout.write((char *)&elem.size, sizeof(Vector2));
        fout.write((char *)&elem.pivot, sizeof(Vector2));
        fout.write((char *)&elem.rotation, sizeof(float));
        writeString(elem.text);
        writeString(elem.fontName);
        fout.write((char *)&elem.fontSize, sizeof(int));
        fout.write((char *)&elem.spacing, sizeof(float));
        fout.write((char *)&elem.textColor, sizeof(Color));
        fout.write((char *)&elem.normalColor, sizeof(Color));
        fout.write((char *)&elem.hoverColor, sizeof(Color));
        fout.write((char *)&elem.pressedColor, sizeof(Color));
        fout.write((char *)&elem.borderRadius, sizeof(float));
        fout.write((char *)&elem.borderWidth, sizeof(float));
        fout.write((char *)&elem.borderColor, sizeof(Color));
        writeString(elem.eventId);
        fout.write((char *)&elem.tint, sizeof(Color));
        writeString(elem.texturePath);
        writeString(elem.scriptPath);
        writeString(elem.actionType);
        writeString(elem.actionTarget);
    }

    fout.close();
    CD_CORE_INFO("Scene serialized to binary: %s", filepath.c_str());
    return true;
}

bool SceneSerializer::DeserializeBinary(const std::string &filepath)
{
    std::ifstream fin(filepath, std::ios::in | std::ios::binary);
    if (!fin.is_open())
        return false;

    SceneHeader header;
    fin.read((char *)&header, sizeof(header));

    if (header.magic[0] != 'C' || header.magic[1] != 'H' || header.magic[2] != 'S' ||
        header.magic[3] != 'C')
    {
        CD_CORE_ERROR("Invalid scene file magic: %s", filepath.c_str());
        return false;
    }

    auto readString = [&]() -> std::string
    {
        uint32_t size;
        fin.read((char *)&size, sizeof(size));
        std::string str(size, '\0');
        fin.read(&str[0], size);
        return str;
    };

    // 1. Metadata
    MapMetadata meta;
    meta.name = readString();
    meta.displayName = readString();
    meta.description = readString();
    meta.author = readString();
    meta.version = readString();
    fin.read((char *)&meta.startPosition, sizeof(Vector3));
    fin.read((char *)&meta.endPosition, sizeof(Vector3));
    fin.read((char *)&meta.skyColor, sizeof(Color));
    fin.read((char *)&meta.groundColor, sizeof(Color));
    fin.read((char *)&meta.difficulty, sizeof(float));
    meta.skyboxTexture = readString();
    fin.read((char *)&meta.sceneType, sizeof(SceneType));
    m_Scene->SetMapMetaData(meta);

    // 2. Objects
    uint32_t objectCount;
    fin.read((char *)&objectCount, sizeof(objectCount));
    std::vector<MapObjectData> objects(objectCount);

    for (uint32_t i = 0; i < objectCount; i++)
    {
        auto &obj = objects[i];
        obj.name = readString();
        fin.read((char *)&obj.type, sizeof(MapObjectType));
        fin.read((char *)&obj.position, sizeof(Vector3));
        fin.read((char *)&obj.rotation, sizeof(Vector3));
        fin.read((char *)&obj.scale, sizeof(Vector3));
        fin.read((char *)&obj.color, sizeof(Color));
        obj.modelName = readString();
        fin.read((char *)&obj.radius, sizeof(float));
        fin.read((char *)&obj.height, sizeof(float));
        fin.read((char *)&obj.size, sizeof(Vector2));
        fin.read((char *)&obj.isPlatform, sizeof(bool));
        fin.read((char *)&obj.isObstacle, sizeof(bool));
        obj.scriptPath = readString();
    }
    m_Scene->AddMapObjects(objects);

    // 3. UI Elements
    uint32_t uiCount;
    fin.read((char *)&uiCount, sizeof(uiCount));
    std::vector<UIElementData> uiElements(uiCount);

    for (uint32_t i = 0; i < uiCount; i++)
    {
        auto &elem = uiElements[i];
        elem.name = readString();
        elem.type = readString();
        fin.read((char *)&elem.isActive, sizeof(bool));
        fin.read((char *)&elem.anchor, sizeof(int));
        fin.read((char *)&elem.position, sizeof(Vector2));
        fin.read((char *)&elem.size, sizeof(Vector2));
        fin.read((char *)&elem.pivot, sizeof(Vector2));
        fin.read((char *)&elem.rotation, sizeof(float));
        elem.text = readString();
        elem.fontName = readString();
        fin.read((char *)&elem.fontSize, sizeof(int));
        fin.read((char *)&elem.spacing, sizeof(float));
        fin.read((char *)&elem.textColor, sizeof(Color));
        fin.read((char *)&elem.normalColor, sizeof(Color));
        fin.read((char *)&elem.hoverColor, sizeof(Color));
        fin.read((char *)&elem.pressedColor, sizeof(Color));
        fin.read((char *)&elem.borderRadius, sizeof(float));
        fin.read((char *)&elem.borderWidth, sizeof(float));
        fin.read((char *)&elem.borderColor, sizeof(Color));
        elem.eventId = readString();
        fin.read((char *)&elem.tint, sizeof(Color));
        elem.texturePath = readString();
        elem.scriptPath = readString();
        elem.actionType = readString();
        elem.actionTarget = readString();
    }
    m_Scene->AddUIElements(uiElements);

    fin.close();
    CD_CORE_INFO("Scene deserialized from binary: %s", filepath.c_str());
    return true;
}

bool SceneSerializer::SerializeJson(const std::string &filepath)
{
    SceneLoader loader;
    return loader.SaveScene(*m_Scene, filepath);
}

bool SceneSerializer::DeserializeJson(const std::string &filepath)
{
    SceneLoader loader;
    *m_Scene = loader.LoadScene(filepath);
    return !m_Scene->GetMapObjects().empty();
}

} // namespace CHEngine
