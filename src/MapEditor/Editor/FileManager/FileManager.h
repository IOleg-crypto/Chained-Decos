//
// Created by Kilo Code
//

#ifndef FILEMANAGER_H
#define FILEMANAGER_H

#include "IFileManager.h"
#include <memory>
#include <string>
#include <vector>
#include "Engine/Map/MapLoader.h"
#include "Engine/MapFileManager/MapFileManager.h"
#include "Engine/MapFileManager/JsonMapFileManager.h"

class FileManager : public IFileManager {
private:
    // File dialog state
    std::string m_currentWorkingDirectory;
    std::vector<std::string> m_currentDirectoryContents;
    std::string m_currentlySelectedFile;
    std::string m_newFileNameInput;
    std::string m_newFolderNameInput;
    bool m_displayFileDialog;
    bool m_isFileLoadDialog;
    bool m_isJsonExportDialog;
    bool m_displayNewFolderDialog;
    bool m_displayDeleteConfirmationDialog;
    std::string m_itemPendingDeletion;

    // Parkour map dialog state
    bool m_displayParkourMapDialog;
    std::vector<GameMap> m_availableParkourMaps;
    int m_currentlySelectedParkourMapIndex;

    // Current file path
    std::string m_currentlyLoadedMapFilePath;

public:
    FileManager();
    ~FileManager() override;

    // File operations
    bool SaveMap(const std::string& filename, const std::vector<MapObject>& objects) override;
    bool LoadMap(const std::string& filename, std::vector<MapObject>& objects) override;
    bool ExportForGame(const std::string& filename, const std::vector<MapObject>& objects) override;
    bool ExportAsJSON(const std::string& filename, const std::vector<MapObject>& objects) override;

    // Parkour map operations
    void LoadParkourMap(const std::string& mapName, std::vector<MapObject>& objects) override;
    void GenerateParkourMap(const std::string& mapName, std::vector<MapObject>& objects) override;
    std::vector<GameMap> GetAvailableParkourMaps() override;
    void ShowParkourMapSelector() override;

    // File dialog operations
    void OpenFileDialog(bool isLoad) override;
    void RenderFileDialog() override;
    void RefreshDirectoryItems() override;
    void NavigateToDirectory(const std::string& path) override;

    // Getters for dialog state
    bool IsFileDialogOpen() const override;
    std::string GetCurrentWorkingDirectory() const override;
    std::string GetCurrentlyLoadedMapFilePath() const override;
    void SetCurrentlyLoadedMapFilePath(const std::string& path) override;

private:
    // Helper methods
    void RenderParkourMapDialog();
};

#endif // FILEMANAGER_H