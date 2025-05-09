#pragma once

#include <SFML/Graphics.hpp>
#include <unordered_map>
#include <memory>
#include <string>
#include <vector>

class TileSetManager {
public:
    bool loadTileset(const std::string& name, const std::string& texturePath);
    const sf::Texture* getTexture(const std::string& name) const;
    const sf::Texture* getTileset(const std::string& name) const;
    sf::Texture* getRawTexture(const std::string& name);
    bool hasTileset(const std::string& name) const;

    void unloadTileset(const std::string& name);
    void clear();
    std::vector<std::string> getLoadedTilesetNames() const;

private:
    std::unordered_map<std::string, std::unique_ptr<sf::Texture>> tilesets;
};