#include "WorldConfig.h"
#include <fstream>
#include <iostream>
#include <algorithm>


WorldConfig loadWorldConfig(const std::string& iniPath) {
    WorldConfig config;
    std::ifstream file(iniPath);
    if (!file.is_open()) {
        std::cerr << "Impossibile aprire il file: " << iniPath << "\n";
        return config;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '[' || line[0] == '#') continue;
        auto sep = line.find('=');
        if (sep == std::string::npos) continue;

        std::string key = line.substr(0, sep);
        std::transform(key.begin(), key.end(), key.begin(), ::tolower);

        std::string value = line.substr(sep + 1);

        if (key == "tileWidth") config.tileWidth = std::stoi(value);
        else if (key == "tileHeight") config.tileHeight = std::stoi(value);
        else if (key == "mapWidth") config.mapWidth = std::stoi(value);
        else if (key == "mapHeight") config.mapHeight = std::stoi(value);
        else if (key == "tileScreenSize") config.tileScreenSize = std::stoi(value);
        else if (key == "tilesetpath") config.tilesetPath = value;
        else if (key == "metadatapath") config.metadataPath = value;
        else if (key == "backgroundcolor") {
            int r, g, b;
            if (sscanf_s(value.c_str(), "%d,%d,%d", &r, &g, &b) == 3)
                config.backgroundColor = sf::Color(r, g, b);
        }


    }

    return config;
}

void saveWorldConfig(const std::string& iniPath, const WorldConfig& config) {
    std::ofstream out(iniPath);
    out << "[World]\n";
    out << "tileWidth=" << config.tileWidth << "\n";
    out << "tileHeight=" << config.tileHeight << "\n";
    out << "mapWidth=" << config.mapWidth << "\n";
    out << "mapHeight=" << config.mapHeight << "\n";
    out << "tilesetPath=" << config.tilesetPath << "\n";
    out << "metadataPath=" << config.metadataPath << "\n";
    out << "tileScreenSize=" << config.tileScreenSize << "\n";
    out << "backgroundColor="
        << (int)config.backgroundColor.r << ","
        << (int)config.backgroundColor.g << ","
        << (int)config.backgroundColor.b << "\n";

}
