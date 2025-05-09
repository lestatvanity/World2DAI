#include "TileSetManager.h"
#include <filesystem>
#include <iostream>

bool TileSetManager::loadTileset(const std::string& name, const std::string& texturePath) {
    auto texture = std::make_unique<sf::Texture>();
    if (!texture->loadFromFile(texturePath)) {
        std::cerr << "[TileSetManager] Errore caricamento texture: " << texturePath << "\n";
        return false;
    }

    texture->setSmooth(false); // evita smoothing per pixel art
    tilesets[name] = std::move(texture);

    std::cout << "[TileSetManager] Caricato tileset '" << name << "' da " << texturePath << "\n";
    return true;
}

const sf::Texture* TileSetManager::getTexture(const std::string& name) const {
    auto it = tilesets.find(name);
    if (it == tilesets.end()) {
        std::cerr << "[TileSetManager::getTexture] Tileset '" << name << "' NON trovato!\n";
        return nullptr;
    }

    if (!it->second) {
        std::cerr << "[TileSetManager::getTexture] Puntatore a texture è nullptr per '" << name << "'!\n";
        return nullptr;
    }

    return it->second.get();
}



sf::Texture* TileSetManager::getRawTexture(const std::string& name) {
    auto it = tilesets.find(name);
    return (it != tilesets.end()) ? it->second.get() : nullptr;
}

bool TileSetManager::hasTileset(const std::string& name) const {
    return tilesets.find(name) != tilesets.end();
}

void TileSetManager::unloadTileset(const std::string& name) {
    auto it = tilesets.find(name);
    if (it != tilesets.end()) {
        tilesets.erase(it);
        std::cout << "[TileSetManager] Tileset '" << name << "' rimosso\n";
    }
}

void TileSetManager::clear() {
    tilesets.clear();
    std::cout << "[TileSetManager] Tutti i tileset sono stati rimossi\n";
}

std::vector<std::string> TileSetManager::getLoadedTilesetNames() const {
    std::vector<std::string> names;
    for (const auto& pair : tilesets) {
        names.push_back(pair.first);
    }
    return names;
}
