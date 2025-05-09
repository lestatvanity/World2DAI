#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <unordered_map>
#include <string>
#include "TileInfo.h"

struct SeasonTileset {
    std::string seasonName; // es. "spring"
    std::string filePath;   // es. "assets/Tileset Grass Spring.png"
    sf::Texture texture;    // caricato da file
};


class TileSelectorPanel {
public:
    TileSelectorPanel();
    void handleInput(const sf::Event& event, sf::Vector2i mousePos);
    void loadTileset(const std::string& tilesetPath);
    void update(float dt, int& selectedTileID);
    void render(sf::RenderWindow& window);
    int getSelectedTileID() const;
    void setSelectedTile(int tileID);
    TileInfo getTileInfo(int id) const;
    std::vector<SeasonTileset> seasonTilesets;
    int selectedSeasonIndex = 0;
    void loadSeasonTilesetsFromFolder(const std::string& folderPath);
    void setTileSize(int width, int height);
    

private:
    sf::Texture tilesetTexture;
    sf::VertexArray tileGrid;
    sf::RectangleShape selectorBox;
    sf::RectangleShape previewDisplay;
    int tileSize = 34;
    int currentTileID = 0;

    std::unordered_map<int, TileInfo> tileMetadata;
    std::string season; // "spring", "summer", "fall", "winter"

    void buildGrid();

    sf::Font font;
    sf::Text tileIDText;

    int maxTileID = 30;
    int columns = 5;
    
    //void buildGrid();
};
