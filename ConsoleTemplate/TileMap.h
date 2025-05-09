#pragma once
#include "TilesetManager.h"
#include "AnimationGroup.h"
#include "SoundManager.h"
#include <SFML/Graphics.hpp>
#include <vector>
#include <memory>
#include <string>

class TileSetManager;

// Nomi dei layer per riferimento
enum LayerType {
    BASE_LAYER = 0,
    DECOR_LAYER = 1,
    ENTITY_LAYER = 2,
    TOP_LAYER = 3
};

struct TileInstance {
    std::string tilesetName; // tipo: "neve", "terra", "dungeon"
    int tileID = -1;

    bool isValid() const { return tileID >= 0 && !tilesetName.empty(); }
};




class TileMap : public sf::Drawable, public sf::Transformable {
public:
    TileMap(int width, int height, int tileSize, TileSetManager* manager);
    // Inserisce un'istanza del gruppo animato a una cella origine
    void addAnimatedGroup(const AnimationGroup& def, const sf::Vector2i& origin);

    // Aggiorna tile animati (se non esiste già la firma)
    void update(float dt);
    bool loadTilesetTexture(const sf::Texture& texture);
    bool loadTilesetTextureFromFile(const std::string& path);
    void setTileSize(int width, int height);
    void setMapSize(int width, int height);

    void setTile(int layer, int x, int y, const TileInstance& tile);
    TileInstance getTile(int layer, int x, int y) const;
    void setTileSetManager(TileSetManager* manager);
    // Deluxe: ottenere lista tileset attivi
    const std::vector<std::string>& getAvailableTilesets() const { return availableTilesets; }
    void addLayer();
    int getLayerCount() const;

    void clear();
    bool loadDebugFont(const std::string& path);
    void enableDebugGrid(bool enable); // nuovo metodo per abilitare overlay
    void floodFill(int layer, int x, int y, const TileInstance& target, const TileInstance& replacement);
    const std::vector<std::string>& getTilesetNames() const; // restituisce i nomi disponibili
    void setCurrentTileset(const std::string& name);          // imposta tileset attivo
    std::string getCurrentTileset() const;                    // ottieni tileset attivo
    void setSoundManager(SoundManager* manager) { soundManager = manager; }
    void clearTile(int layer, int x, int y);
    std::vector<std::vector<std::vector<TileInstance>>> getFullMapState() const;
    void setFullMapState(const std::vector<std::vector<std::vector<TileInstance>>>& state);

private:
    virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const override;
    sf::Font debugFont;

    const sf::Texture* tilesetTexture = nullptr;
    sf::Texture internalTexture;
    int tileWidth = 32;
    int tileHeight = 32;
    int mapWidth = 0;
    int mapHeight = 0;

    bool debugGrid = false;

    // Struttura: layers[layer][y][x] = tileID
    std::vector<std::vector<std::vector<TileInstance>>> layers;
    TileSetManager* tileSetManager = nullptr;

    std::vector<std::string> availableTilesets; // Deluxe addition
    std::vector<std::string> tilesetNames; // elenco nomi tileset
    std::string currentTileset;            // tileset attivo per il brush
    // Collezione di gruppi animati piazzati sulla mappa
    std::vector<AnimationGroupInstance> animatedGroups;
    SoundManager* soundManager = nullptr;


};

