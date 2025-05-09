#pragma once

#include <SFML/Graphics.hpp>
#include <string>
#include <unordered_map>
#include <vector>
#include "TileInfo.h"
#include "TileMap.h"
#include <imgui_internal.h>
#include <functional>
class TileSetManager;

struct TilesetEntry {
    std::string name;
    std::string path;
    std::string season = "tutte";
};





//extern int tileConfigID;
//extern std::unordered_map<int, TileInfo> tileMetadata;


class TilesetEditorPanel {
public:
    TilesetEditorPanel(TileSetManager& tilesetManager);

    void render(bool* open);
    void setTileSize(int width, int height);
    std::string getSelectedTilesetName() const { return selectedTilesetName; }
    std::function<void(const std::string&)> onTilesetSelected;
    std::function<void(int)> onTileSelected; // ✅ per sincronizzare con EditorState
    void setTileMap(TileMap* tileMap) {
        this->tileMap = tileMap;
    }
    void renderTileProperties();
    void renderTileGridSection();
    void loadMetadataFromJson(const std::string& path);
    void saveAnimationGroupsToJson(const std::string& path);
    void loadAnimationGroupsFromJson(const std::string& path);
    void renderAnimationEditorSection();
    //void renderAnimationGroupEditor();
    //void renderAnimationPreviewSection();
    void renderCompositeAnimationPreview();
    void renderAnimationGroupPlacement();
    //void renderInteractiveAnimationGroupEditor();
private:
    TileSetManager& tilesetManager;

    int selectedTileID = -1;
    int selectedTilesetIndex = 0;
    int tileWidth = 16;
    int tileHeight = 16;

    std::string selectedTilesetName;
    std::unordered_map<int, TileInfo> tileProperties; // ID -> proprietà
    std::vector<TilesetEntry> loadedTilesets;

    TileMap* tileMap = nullptr;
    void drawTileGrid();
    void editTileProperties();
    void saveMetadataToJson(const std::string& path);
    void generateLuaScript(const std::string& path);
    void openTilesetFileDialog();
    void loadTilesetFromFile(const std::string& filePath);
};