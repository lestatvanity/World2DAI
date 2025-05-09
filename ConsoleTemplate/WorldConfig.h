#pragma once
#include <SFML/Graphics/Color.hpp>  // serve per sf::Color
#include <string>
#include <array>

struct WorldConfig {
    int tileWidth = 16;
    int tileHeight = 16;
    int mapWidth = 100;
    int mapHeight = 100;
    int tileScreenSize = 48;
    
    std::string tilesetPath = "assets/tileset.png";
    std::string metadataPath = "assets/metadata.json";

    sf::Color backgroundColor = sf::Color(100, 100, 100); // colore iniziale
};

WorldConfig loadWorldConfig(const std::string& iniPath);
void saveWorldConfig(const std::string& iniPath, const WorldConfig& config);

