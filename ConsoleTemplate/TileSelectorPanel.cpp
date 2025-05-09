#include "imgui_includes.h"
#include "TileSelectorPanel.h"
#include <fstream>
#include <iostream>
#include <cmath>
#include <nlohmann/json.hpp>


using json = nlohmann::json;


TileSelectorPanel::TileSelectorPanel() {
    selectorBox.setSize({ 17.f, 17.f });
    selectorBox.setFillColor(sf::Color::Transparent);
    selectorBox.setOutlineColor(sf::Color::Red);
    selectorBox.setOutlineThickness(2.f);

    previewDisplay.setSize({ 34.f, 34.f });
    previewDisplay.setPosition(10.f, 260.f);

    if (!font.loadFromFile("assets/arial.ttf")) {
        std::cerr << "Errore nel caricamento del font del selettore tile\n";
    }

    tileIDText.setFont(font);
    tileIDText.setCharacterSize(14);
    tileIDText.setFillColor(sf::Color::White);
    tileIDText.setPosition(10.f, 220.f);
}

void TileSelectorPanel::loadSeasonTilesetsFromFolder(const std::string& folderPath) {
    seasonTilesets.clear();

    std::vector<std::string> seasonNames = { "spring", "summer", "fall", "winter" };

    for (const auto& season : seasonNames) {
        std::string capitalized = season;
        capitalized[0] = static_cast<char>(std::toupper(capitalized[0]));

        std::string path = folderPath + "/Tileset_Grass " + capitalized + ".png";        sf::Texture tex;
        if (tex.loadFromFile(path)) {
            SeasonTileset tileset{ season, path, tex };
            seasonTilesets.push_back(tileset);
            std::cout << "[TilePanel] Caricato: " << path << "\n";
        }
        else {
            std::cerr << "[TilePanel] Impossibile caricare: " << path << "\n";
        }
    }
}


void TileSelectorPanel::loadTileset(const std::string& tilesetFolder) {
    std::string texturePath = tilesetFolder + "/tileset.png";
    std::string metadataPath = tilesetFolder + "/metadata.json";

    if (!tilesetTexture.loadFromFile(texturePath)) {
        std::cerr << "[TileSelectorPanel] Errore nel caricamento texture: " << texturePath << "\n";
        return;
    }

    // Settiamo la texture
    previewDisplay.setTexture(&tilesetTexture);

    // Carica metadata
    std::ifstream file(metadataPath);
    if (file.is_open()) {
        try {
            json j;
            file >> j;
            tileMetadata.clear();
            for (auto it = j.begin(); it != j.end(); ++it) {
                int id = std::stoi(it.key());
                TileInfo info;
                info.name = it.value().value("name", "Sconosciuto");
                info.type = it.value().value("type", "unknown");
                info.walkable = it.value().value("walkable", true);
                tileMetadata[id] = info;
            }
        }
        catch (const std::exception& e) {
            std::cerr << "[TileSelectorPanel] Errore parsing metadata: " << e.what() << "\n";
        }
    }
    else {
        std::cerr << "[TileSelectorPanel] Impossibile aprire: " << metadataPath << "\n";
    }

    buildGrid();
}


void TileSelectorPanel::buildGrid() {
    int columns = 10; // o automatico
    tileGrid.setPrimitiveType(sf::Quads);
    tileGrid.resize(maxTileID * 4);

    

    for (int i = 0; i < maxTileID; ++i) {
        int col = i % columns;
        int row = i / columns;

        float tx = static_cast<float>(col * tileSize);
        float ty = static_cast<float>(row * tileSize);

        sf::Vertex* quad = &tileGrid[i * 4];

        quad[0].position = sf::Vector2f(10 + col * (tileSize + 2), 10 + row * (tileSize + 2));
        quad[1].position = sf::Vector2f(10 + col * (tileSize + 2) + tileSize, 10 + row * (tileSize + 2));
        quad[2].position = sf::Vector2f(10 + col * (tileSize + 2) + tileSize, 10 + row * (tileSize + 2) + tileSize);
        quad[3].position = sf::Vector2f(10 + col * (tileSize + 2), 10 + row * (tileSize + 2) + tileSize);

        quad[0].texCoords = sf::Vector2f(tx, ty);
        quad[1].texCoords = sf::Vector2f(tx + tileSize, ty);
        quad[2].texCoords = sf::Vector2f(tx + tileSize, ty + tileSize);
        quad[3].texCoords = sf::Vector2f(tx, ty + tileSize);
    }
}

void TileSelectorPanel::update(float dt, int& selectedTileID) {
    sf::Vector2i mouse = sf::Mouse::getPosition();
    sf::Vector2f pos(static_cast<float>(mouse.x), static_cast<float>(mouse.y));

    


    if (sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
        for (int i = 0; i < maxTileID; ++i) {
            int col = i % columns;
            int row = i / columns;
            sf::FloatRect tileArea(10 + col * (tileSize + 2), 10 + row * (tileSize + 2), tileSize, tileSize);
            if (tileArea.contains(pos)) {
                selectedTileID = i;
                selectorBox.setPosition(10 + col * (tileSize + 2), 10 + row * (tileSize + 2));
            }
        }
    }

    int col = selectedTileID % columns;
    int row = selectedTileID / columns;
    previewDisplay.setTextureRect(sf::IntRect(col * tileSize, row * tileSize, tileSize, tileSize));
    tileIDText.setString("Tile selezionato: " + std::to_string(selectedTileID));
}

void TileSelectorPanel::render(sf::RenderWindow& window) {
    sf::RenderStates states;
    states.texture = &tilesetTexture;

    window.draw(tileGrid, states);
    window.draw(selectorBox);
    window.draw(tileIDText);
    window.draw(previewDisplay);
}


int TileSelectorPanel::getSelectedTileID() const {
    return currentTileID;
}

TileInfo TileSelectorPanel::getTileInfo(int id) const {
    auto it = tileMetadata.find(id);
    if (it != tileMetadata.end()) {
        return it->second;
    }
    return { "Sconosciuto", "Unknown", true };
}

void TileSelectorPanel::setTileSize(int width, int height) {
    tileSize = width;
    selectorBox.setSize({ (float)width, (float)height });
    previewDisplay.setSize({ (float)width, (float)height });
}
