#include "TileMap.h"
#include "TileSetManager.h"
#include <iostream>
#include <filesystem>
#include <stack>

TileMap::TileMap(int width, int height, int tileSize, TileSetManager* manager)
    : mapWidth(width), mapHeight(height), tileWidth(tileSize), tileHeight(tileSize), tileSetManager(manager) {
    this->tileSetManager = tileSetManager;

    // Prepara i layer
    layers.resize(4); // base, decor, entity, top
    for (auto& layer : layers) {
        layer.resize(mapHeight, std::vector<TileInstance>(mapWidth, TileInstance{})); // inizializzati vuoti

    }
    //Inizializza vettore gruppi animati (vuoto)
    animatedGroups.clear();
}

void TileMap::addAnimatedGroup(const AnimationGroup& def, const sf::Vector2i& origin) {
    animatedGroups.push_back({
    &def,
    origin,
    0.f });
}

//──────────────────────────────────────────────
//  Update generale (chiama da Game::update)
//──────────────────────────────────────────────
void TileMap::update(float dt) {
        // Avanza tutti i gruppi
        for (auto& g : animatedGroups) {
        g.time += dt;
        const int frameIndex = static_cast<int>(g.time / g.def->frameDuration) % g.def->frames.size();
        
                    // per ogni tile nel frame corrente copia il tileID nel layer desiderato
            for (const auto& f : g.def->frames[frameIndex]) {
            const int x = g.origin.x + f.offset.x;
            const int y = g.origin.y + f.offset.y;
            
                if (x < 0 || x >= mapWidth || y < 0 || y >= mapHeight) continue;
            
                layers[DECOR_LAYER][y][x].tileID = f.tileID;
                layers[DECOR_LAYER][y][x].tilesetName = currentTileset; // oppure fissa a def.tilesetName se lo aggiungi
            
            }

            if (!g.playing && !g.def->soundName.empty()) {
                soundManager->playSound(g.def->soundName, g.def->soundLoop, g.def->soundVolume);
                g.playing = true;
            }

        
    }

       
    
           // … eventuale altra logica di update (entità, timers) …
        
}

bool TileMap::loadTilesetTexture(const sf::Texture& texture) {
    tilesetTexture = &texture;
    return true;
}

bool TileMap::loadTilesetTextureFromFile(const std::string& path) {
    if (!internalTexture.loadFromFile(path)) {
        std::cerr << "[TileMap] Errore nel caricamento della texture da file: " << path << "\n";
        return false;
    }
    tilesetTexture = &internalTexture;
    return true;
}


void TileMap::setTileSize(int width, int height) {
    tileWidth = width;
    tileHeight = height;
}

void TileMap::setMapSize(int width, int height) {
    mapWidth = width;
    mapHeight = height;
    for (auto& layer : layers) {
        layer.resize(mapHeight, std::vector<TileInstance>(mapWidth));
    }
}

void TileMap::addLayer() {
    std::vector<std::vector<TileInstance>> newLayer(mapHeight, std::vector<TileInstance>(mapWidth));
    layers.push_back(newLayer);
}

int TileMap::getLayerCount() const {
    return static_cast<int>(layers.size());
}

void TileMap::setTile(int layer, int x, int y, const TileInstance& tile) {
    std::cout << "[SET TILE] layer=" << layer << ", x=" << x << ", y=" << y
        << " tileset=" << tile.tilesetName << ", id=" << tile.tileID << "\n";

    if (!tileSetManager) {
        std::cerr << "[CRASH] tileSetManager è nullptr in setTile!\n";
        return;
    }

    if (layer < 0 || layer >= layers.size() || x < 0 || x >= mapWidth || y < 0 || y >= mapHeight) {
        std::cerr << "[CRASH PREVENT] setTile: coordinate fuori limite layer=" << layer
            << ", x=" << x << ", y=" << y << "\n";
        return;
    }

    if (!tileSetManager->hasTileset(tile.tilesetName)) {
        std::cerr << "[CRASH PREVENT] Tileset '" << tile.tilesetName << "' non trovato!\n";
        return;
    }

    const sf::Texture* tex = tileSetManager->getTexture(tile.tilesetName);
    

    int tilesetColumns = tex->getSize().x / tileWidth;
    int tilesetRows = tex->getSize().y / tileHeight;
    int maxTileID = tilesetColumns * tilesetRows;
    

    layers[layer][y][x] = tile;
}


TileInstance TileMap::getTile(int layer, int x, int y) const {



    if (layer >= 0 && layer < layers.size() &&
        x >= 0 && x < mapWidth &&
        y >= 0 && y < mapHeight) {
        return layers[layer][y][x];
    }

    // Fallback di sicurezza
    return TileInstance{}; // restituisce un TileInstance "vuoto" (tileID = -1, nome vuoto)
}


void TileMap::clear() {
    layers.clear();
}

void TileMap::enableDebugGrid(bool enable) {
    debugGrid = enable;
}

void TileMap::setTileSetManager(TileSetManager* manager) {
    this->tileSetManager = manager;
}


bool TileMap::loadDebugFont(const std::string& path) {
    if (!debugFont.loadFromFile(path)) {
        std::cerr << "[DEBUG] Impossibile caricare font debug da: " << path << "\n";
        return false;
    }
    return true;
}


void TileMap::draw(sf::RenderTarget& target, sf::RenderStates states) const {
    states.transform *= getTransform();

    if (!tileSetManager) {
        std::cerr << "[TileMap::draw] tileSetManager è nullptr!\n";
        return;
    }

    for (size_t layerIndex = 0; layerIndex < layers.size(); ++layerIndex) {
        const auto& layer = layers[layerIndex];

        if (layerIndex > 0 && std::all_of(layer.begin(), layer.end(), [](const auto& row) {
            return std::all_of(row.begin(), row.end(), [](const TileInstance& t) { return !t.isValid(); });
            })) continue;

        std::unordered_map<std::string, sf::VertexArray> batchMap;

        for (int y = 0; y < mapHeight; ++y) {
            for (int x = 0; x < mapWidth; ++x) {
                if (y >= static_cast<int>(layer.size()) || x >= static_cast<int>(layer[y].size()))
                    continue;

                const TileInstance& tile = layer[y][x];
                if (!tile.isValid()) continue;

                if (!tileSetManager->hasTileset(tile.tilesetName)) {
                    std::cerr << "[DRAW] Tileset mancante: '" << tile.tilesetName << "'\n";
                    continue;
                }

                const sf::Texture* tex = tileSetManager->getTexture(tile.tilesetName);
                if (!tex || tileWidth == 0 || tileHeight == 0) continue;

                int columns = tex->getSize().x / tileWidth;
                int rows = tex->getSize().y / tileHeight;
                int maxTileID = columns * rows;
                if (tile.tileID >= maxTileID) {
                    std::cerr << "[DRAW] Tile ID fuori range! ID=" << tile.tileID << ", max=" << maxTileID << "\n";
                    continue;
                }

                int tu = tile.tileID % columns;
                int tv = tile.tileID / columns;

                sf::VertexArray& batch = batchMap[tile.tilesetName];
                if (batch.getPrimitiveType() != sf::Quads)
                    batch.setPrimitiveType(sf::Quads);

                sf::Vertex quad[4];
                quad[0].position = sf::Vector2f(x * tileWidth, y * tileHeight);
                quad[1].position = sf::Vector2f((x + 1) * tileWidth, y * tileHeight);
                quad[2].position = sf::Vector2f((x + 1) * tileWidth, (y + 1) * tileHeight);
                quad[3].position = sf::Vector2f(x * tileWidth, (y + 1) * tileHeight);

                quad[0].texCoords = sf::Vector2f(tu * tileWidth, tv * tileHeight);
                quad[1].texCoords = sf::Vector2f((tu + 1) * tileWidth, tv * tileHeight);
                quad[2].texCoords = sf::Vector2f((tu + 1) * tileWidth, (tv + 1) * tileHeight);
                quad[3].texCoords = sf::Vector2f(tu * tileWidth, (tv + 1) * tileHeight);

                for (int i = 0; i < 4; ++i)
                    batch.append(quad[i]);
            }
        }

        for (const auto& [tilesetName, vertices] : batchMap) {
            const sf::Texture* tex = tileSetManager->getTexture(tilesetName);
            if (!tex) {
                std::cerr << "[DRAW] Texture mancante per tileset " << tilesetName << "\n";
                continue;
            }

            states.texture = tex;
            target.draw(vertices, states);
        }
    }
}





void TileMap::floodFill(int layer, int x, int y, const TileInstance& target, const TileInstance& replacement) {
    if (!target.isValid() || target.tilesetName == replacement.tilesetName && target.tileID == replacement.tileID)
        return;
    if (getTile(layer, x, y).tileID != target.tileID || getTile(layer, x, y).tilesetName != target.tilesetName)
        return;

    std::stack<std::pair<int, int>> toFill;
    toFill.push({ x, y });

    while (!toFill.empty()) {
        auto [cx, cy] = toFill.top();
        toFill.pop();

        if (cx < 0 || cx >= mapWidth || cy < 0 || cy >= mapHeight)
            continue;

        TileInstance current = getTile(layer, cx, cy);
        if (current.tilesetName != target.tilesetName || current.tileID != target.tileID)
            continue;

        setTile(layer, cx, cy, replacement);

        toFill.push({ cx + 1, cy });
        toFill.push({ cx - 1, cy });
        toFill.push({ cx, cy + 1 });
        toFill.push({ cx, cy - 1 });
    }
}
const std::vector<std::string>& TileMap::getTilesetNames() const {
    return tilesetNames;
}

void TileMap::setCurrentTileset(const std::string& name) {
    currentTileset = name;
}

std::string TileMap::getCurrentTileset() const {
    return currentTileset;
}


void TileMap::clearTile(int layer, int x, int y) {
    if (layer >= 0 && layer < layers.size() &&
        y >= 0 && y < layers[layer].size() &&
        x >= 0 && x < layers[layer][y].size()) {
        layers[layer][y][x] = TileInstance(); // Tile vuoto
    }
}


std::vector<std::vector<std::vector<TileInstance>>> TileMap::getFullMapState() const {
    return layers;
}

void TileMap::setFullMapState(const std::vector<std::vector<std::vector<TileInstance>>>& state) {
    layers = state;
}
