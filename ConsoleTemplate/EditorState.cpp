#define USE_STD_FILESYSTEM

#include "imgui_includes.h"
#include "ImGuiFileDialog.h"
#include "EditorState.h"
#include "Game.h"
#include "InGameState.h"
#include "MenuState.h"
//#include "WorldConfig.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <SFML/System.hpp>
#include <imgui_internal.h>
#include <cmath>
#include <nlohmann/json.hpp>
#include <memory>
#include <vector>
#include <unordered_set>





using json = nlohmann::json;
namespace fs = std::filesystem;

extern std::unordered_map<std::string, Entity*> allEntities;

extern bool showWorldSettings;

std::string extractFolderNameFromPath(const std::string& path) {
    return std::filesystem::path(path).filename().string();
}


EditorState::EditorState(Game& game)
    : GameState(game), currentTileID(0), selectedTileDisplay(), currentTilesetIndex(0) {
    // Lascia tutto vuoto, inizializzerai dopo con initFromConfig
    EntityPropertyPanel propertyPanel;
    //worldSettings = nullptr;
    //if (!worldSettings) {
    //    worldSettings = std::make_unique<WorldSettingsManager>(config, game);
   // }

    //tilesetEditor = std::make_unique<TilesetEditorPanel>(tilesetManager);
    //tilesetEditor->setTileSize(
    //   worldSettings->getConfig().tileWidth,
     //   worldSettings->getConfig().tileHeight
    //);

    // 1) crea il pannello
    eventEditorPanel = std::make_unique<EventEditorPanel>();

    // 2) passagli il pointer all’ EventSystem
    eventEditorPanel->setEventSystem(&eventSystem);


    playerPanel.emplace(animationManager, previewComponent); // CORRETTO;

}


void EditorState::initFromConfig(const std::string& configPath) {
    this->configPath = configPath;

    config = loadWorldConfig(configPath);

    std::filesystem::path basePath = std::filesystem::path(configPath).parent_path();

    if (config.mapWidth <= 0 || config.mapHeight <= 0 || config.tileWidth <= 0 || config.tileHeight <= 0) {
        std::cerr << "[EditorState] Configurazione non valida in " << configPath << "\n";
        return;
    }

    if (!worldSettings) {
        worldSettings = std::make_unique<WorldSettingsManager>(config, game);
    }

    tileSetManager = std::make_unique<TileSetManager>();
    tilemap = std::make_unique<TileMap>(config.mapWidth, config.mapHeight, config.tileWidth, tileSetManager.get());

    tileSize = config.tileWidth;
    mapWidth = config.mapWidth;
    mapHeight = config.mapHeight;

    tilesetEditor = std::make_unique<TilesetEditorPanel>(*tileSetManager);
    tilesetEditor->setTileMap(tilemap.get());
    tilesetEditor->setTileSize(config.tileWidth, config.tileHeight);
    tileSetManager->loadTileset(currentTilesetName, config.tilesetPath);
    // PRIMA carica tutti i tileset disponibili!
    loadAvailableTilesets("assets/stagioni");

    //  Callback corretta con stringa
    tilesetEditor->onTilesetSelected = [this](const std::string& name) {
        if (tilemap)
            tilemap->setCurrentTileset(name);
        currentTilesetName = name;
        };

    //  Opzionale: callback per selezione tile
    tilesetEditor->onTileSelected = [this](int tileID) {
        currentTileID = tileID;
        };

    //tilesetEditor->setTileSize(config.tileWidth, config.tileHeight);

    

    //loadAvailableTilesets("assets/stagioni");

    if (!tileSetManager->getLoadedTilesetNames().empty()) {
        std::string defaultTileset = tileSetManager->getLoadedTilesetNames()[0];
        //tilemap->setCurrentTileset(defaultTileset);
        currentTilesetName = defaultTileset;
    }

    if (!tileSelectorTexture.loadFromFile(config.tilesetPath)) {
        std::cerr << "[EditorState] Errore nel caricamento tileset: " << config.tilesetPath << "\n";
    }
    tilePreview.setTexture(tileSelectorTexture);
    selectedTileDisplay.setTexture(&tileSelectorTexture);
    tilemap->loadTilesetTextureFromFile(config.tilesetPath);

    selectedTileDisplay.setSize({ 64.f, 64.f });
    selectedTileDisplay.setPosition(10.f, 260.f);

    if (!fs::exists(config.metadataPath)) {
        std::ofstream createEmpty(config.metadataPath);
        createEmpty << "{}";
        std::cout << "[EditorState] File metadata.json non trovato. Creato nuovo vuoto.\n";
    }
    loadTileMetadata(config.metadataPath);

    if (!font.loadFromFile("assets/arial.ttf")) {
        std::cerr << "[EditorState] Errore nel caricamento font: assets/arial.ttf\n";
    }

    selectedTileBox.setSize({ (float)tileSize, (float)tileSize });
    selectedTileBox.setFillColor(sf::Color::Transparent);
    selectedTileBox.setOutlineColor(sf::Color::Red);
    selectedTileBox.setOutlineThickness(2.f);

    tileIDText.setFont(font);
    tileIDText.setCharacterSize(14);
    tileIDText.setFillColor(sf::Color::White);
    tileIDText.setPosition(10.f, 220.f);

    tileDescText.setFont(font);
    tileDescText.setCharacterSize(14);
    tileDescText.setFillColor(sf::Color::White);
    tileDescText.setPosition(80.f, 260.f);

    game.updateView();
    float scaleFactor = config.tileScreenSize / static_cast<float>(config.tileWidth);
    game.getGameView().zoom(1.0f / scaleFactor);

    sf::Vector2u texSize = tileSelectorTexture.getSize();
    maxTileID = (texSize.x / tileSize) * (texSize.y / tileSize);
    
    buildTileSelectorGrid();

    groupEditor = std::make_unique<AnimationGroupEditorPanel>(*tileSetManager);
    groupEditor->setTileSize(tileSize, tileSize);
    groupEditor->setTilesetName(currentTilesetName); // se già disponibile
    groupEditor->loadFromFile("assets/animation_groups.json"); // AGGIUNGI QUESTO

    

    validateAnimationGroups();

    groupEditor->onGroupPlaced = [this](const std::string& groupName, sf::Vector2i position) {
        placedGroups.push_back({ groupName, position, currentLayerIndex });
        for (const auto& group : groupEditor->getGroups()) {
            if (group.name == groupName) {
                tilemap->addAnimatedGroup(group, position); // piazza effettivamente il gruppo
                break;
            }
        }
        };


    soundManager = std::make_unique<SoundManager>();
    soundManager->loadSound("acqua", "assets/suoni/water.wav"); // esempio
    scriptManager.setSoundManager(soundManager.get()); // collega allo ScriptManager

    propertyPanel.setScriptManager(&scriptManager);
    eventSystem.loadFromFile((basePath / "map.events.json").string());

    

}


void EditorState::validateAnimationGroups() {
    for (auto it = groupEditor->getGroups().begin(); it != groupEditor->getGroups().end(); ) {
        const auto& group = *it;
        const sf::Texture* tex = tileSetManager->getTexture(currentTilesetName);
        if (!tex) {
            std::cerr << "[EditorState] Rimosso gruppo " << group.name << " (tileset mancante)\n";
            it = groupEditor->getGroups().erase(it);
        }
        else {
            ++it;
        }
    }
}



void EditorState::loadFullMap(const std::string& mapName) {
    namespace fs = std::filesystem;
    std::string basePath = "assets/maps/" + mapName + "/";

    if (!fs::exists(basePath)) {
        std::cerr << "[Editor] Errore: la mappa '" << mapName << "' non esiste!\n";
        return;
    }

    loadMap(basePath + "map.txt");
    loadPlacedGroups(basePath + "placed_groups.json");
    groupEditor->loadFromFile(basePath + "animation_groups.json");


    // (In futuro) Carica anche NPC o altri elementi
    // loadNpcObjects(basePath + "npc_objects.json");

    scriptManager.loadZoneScripts(basePath); // (carica eventuali tile_events.lua, zone_events.lua)

    std::cout << "[Editor] Mappa '" << mapName << "' caricata COMPLETAMENTE.\n";
}

void EditorState::saveFullMap(const std::string& mapName) {
    namespace fs = std::filesystem;
    std::string basePath = "assets/maps/" + mapName + "/";

    // Crea cartella se non esiste
    if (!fs::exists(basePath)) {
        fs::create_directories(basePath);
    }

    // Salva dati principali
    saveMap(basePath + "map.txt");
    savePlacedGroups(basePath + "placed_groups.json");
    groupEditor->saveToFile(basePath + "animation_groups.json");


    // (In futuro) Salva anche NPC o altri elementi
    // saveNpcObjects(basePath + "npc_objects.json");

    std::cout << "[Editor] Mappa '" << mapName << "' salvata COMPLETAMENTE.\n";
}


/*void EditorState::loadTileMetadata(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Impossibile aprire il file di metadata: " << path << std::endl;
        return;
    }

    try {
        json j;
        file >> j;
        for (auto it = j.begin(); it != j.end(); ++it) {
            int id = std::stoi(it.key());
            const json& value = it.value();

            TileInfo info;
            info.name = value.value("name", "Sconosciuto");
            info.type = value.value("type", "Unknown");
            info.walkable = value.value("walkable", true);
            tileMetadata[id] = info;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Errore parsing metadata: " << e.what() << std::endl;
    }
}*/

void EditorState::buildTileSelectorGrid() {
    int columns = 5;
    int rows = static_cast<int>(std::ceil(maxTileID / static_cast<float>(columns)));
    tileSelectorGrid.setPrimitiveType(sf::Quads);
    tileSelectorGrid.resize(maxTileID * 4);

    int tileScreenSize = 48; // 👈 dimensione visiva (non cambia la texture)

    for (int i = 0; i < maxTileID; ++i) {
        int col = i % columns;
        int row = i / columns;

        float tx = static_cast<float>((i % (tileSelectorTexture.getSize().x / tileSize)) * tileSize);
        float ty = static_cast<float>((i / (tileSelectorTexture.getSize().x / tileSize)) * tileSize);

        sf::Vertex* quad = &tileSelectorGrid[i * 4];

        // POSIZIONE VISIVA: tileScreenSize
        quad[0].position = sf::Vector2f(10 + col * (tileScreenSize + 2), 10 + row * (tileScreenSize + 2));
        quad[1].position = sf::Vector2f(10 + col * (tileScreenSize + 2) + tileScreenSize, 10 + row * (tileScreenSize + 2));
        quad[2].position = sf::Vector2f(10 + col * (tileScreenSize + 2) + tileScreenSize, 10 + row * (tileScreenSize + 2) + tileScreenSize);
        quad[3].position = sf::Vector2f(10 + col * (tileScreenSize + 2), 10 + row * (tileScreenSize + 2) + tileScreenSize);

        // UV PRECISI (rimangono 16x16)
        quad[0].texCoords = sf::Vector2f(tx, ty);
        quad[1].texCoords = sf::Vector2f(tx + tileSize, ty);
        quad[2].texCoords = sf::Vector2f(tx + tileSize, ty + tileSize);
        quad[3].texCoords = sf::Vector2f(tx, ty + tileSize);
    }
}




void EditorState::loadTilesetFromFolder(const std::string& path) {
    using namespace std::filesystem;

    if (!is_directory(path)) {
        std::cerr << "[TilesetLoader] Percorso non valido: " << path << "\n";
        return;
    }

    int loaded = 0;

    for (const auto& entry : directory_iterator(path)) {
        if (!entry.is_regular_file()) continue;

        std::string extension = entry.path().extension().string();
        std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

        if (extension != ".png") continue;

        std::string pngPath = entry.path().string();
        std::string name = entry.path().stem().string(); // nome senza estensione
        std::string metadataPath = entry.path().parent_path().string() + "/" + name + ".json";

        // Carica immagine come texture con trasparenza
        sf::Image img;
        if (!img.loadFromFile(pngPath)) {
            std::cerr << "[TilesetLoader] Errore nel caricamento PNG: " << pngPath << "\n";
            continue;
        }

        img.createMaskFromColor(sf::Color(255, 0, 255)); // magenta -> trasparente
        sf::Texture texture;
        texture.loadFromImage(img);

        tileSetManager->loadTileset(name, pngPath);
        std::cout << "[TilesetLoader] Caricato tileset: " << name << "\n";

        // Se è il primo, imposta come attivo
        if (loaded == 0) {
            currentTilesetName = name;
            tilemapTexture.loadFromImage(img);
            tilemapTexture.setSmooth(false);
            tilemap->loadTilesetTexture(tilemapTexture);
            tilemap->setCurrentTileset(name);
            tilePreview.setTexture(tilemapTexture);
            selectedTileDisplay.setTexture(&tilemapTexture);
            tilemap->setSoundManager(soundManager.get());


            // Calcola maxTileID
            sf::Vector2u texSize = tilemapTexture.getSize();
            maxTileID = (texSize.x / tileSize) * (texSize.y / tileSize);
            buildTileSelectorGrid();

            // Carica metadata se esiste
            if (fs::exists(metadataPath)) {
                loadTileMetadata(metadataPath);
            }
            else {
                std::cerr << "[TilesetLoader] Metadata mancante: " << metadataPath << "\n";
            }
        }

        loaded++;
    }

    if (loaded == 0) {
        std::cerr << "[TilesetLoader] Nessun tileset caricato da " << path << "\n";
    }
    else {
        std::cout << "[TilesetLoader] Totale tileset caricati: " << loaded << "\n";
    }
}



void EditorState::handleInput(sf::Event& event) {
    if (!tilemap || !tileSetManager || !tileSetManager->hasTileset(currentTilesetName)) {
        return;
    }

    sf::Vector2i pixelPos = sf::Mouse::getPosition(game.getWindow());
    sf::Vector2f worldPos = game.getWindow().mapPixelToCoords(pixelPos, game.getGameView());
    int x = static_cast<int>(worldPos.x / tileSize);
    int y = static_cast<int>(worldPos.y / tileSize);

    if (x < 0 || x >= mapWidth || y < 0 || y >= mapHeight) {
        return;
    }

    if (event.type == sf::Event::KeyPressed) {
        if (event.key.code == sf::Keyboard::Home) {
            sf::Vector2f center(mapWidth * tileSize / 2.f, mapHeight * tileSize / 2.f);
            sf::Vector2u mapPixelSize(mapWidth * tileSize, mapHeight * tileSize);
            game.setViewCenterClamp(center, mapPixelSize);
        }

        if (event.key.code == sf::Keyboard::Escape) {
            if (isSelectingArea || selectionActive) {
                isSelectingArea = false;
                selectionActive = false;
                selectionStart = { -1, -1 };
                selectionEnd = { -1, -1 };
            }
            else {
                game.changeState(std::make_unique<EditorState>(game));
            }
        }


        if (event.key.code == sf::Keyboard::Tab) {
            if (!tilesetPaths.empty()) {
                currentTilesetIndex = (currentTilesetIndex + 1) % tilesetPaths.size();
                currentTileID = 0;
                loadTilesetFromFolder(tilesetPaths[currentTilesetIndex]);
            }
        }

        if (event.key.code == sf::Keyboard::S && event.key.control) {
            saveFullMap("Mappa");
        }

        if (event.key.code == sf::Keyboard::L && event.key.control) {
            loadFullMap("Mappa");
        }

        if (event.key.code == sf::Keyboard::Add || event.key.code == sf::Keyboard::Equal) {
            game.adjustZoom(0.9f);
        }

        if (event.key.code == sf::Keyboard::Subtract || event.key.code == sf::Keyboard::Hyphen) {
            game.adjustZoom(1.1f);
        }

        if (event.key.code == sf::Keyboard::Num0) {
            gameView.setSize(originalViewSize);
            currentZoom = 1.0f;
        }

        if (event.key.code == sf::Keyboard::F11) {
            game.toggleFullscreen();
        }

        if (event.key.code == sf::Keyboard::Delete) {
            // Tasto Canc -> Cancella
            if (selectionActive) {
                deleteTilesAndGroupsInSelection();
                selectionActive = false; // Quando cancello, sparisce la selezione
                selectionStart = { -1, -1 };
                selectionEnd = { -1, -1 };
            }
            else {
                deleteTileAndGroupAt(x, y);
            }

        }

        if (event.type == sf::Event::KeyPressed) {
            if (event.key.control && event.key.code == sf::Keyboard::Z) {
                undo();
            }
            if (event.key.control && event.key.code == sf::Keyboard::Y) {
                redo();
            }
        }

    }

    if (event.type == sf::Event::MouseWheelScrolled) {
        if (ImGui::GetIO().WantCaptureMouse) {
            // Se stiamo usando una finestra ImGui, la rotellina è per scorrere ImGui.
            return;
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::LControl) || sf::Keyboard::isKeyPressed(sf::Keyboard::RControl)) {
            // Se Ctrl è premuto, cambia tile selezionato!
            if (event.mouseWheelScroll.delta > 0) {
                currentTileID = (currentTileID + 1) % maxTileID;
            }
            else if (event.mouseWheelScroll.delta < 0) {
                currentTileID = (currentTileID - 1 + maxTileID) % maxTileID;
            }

            int col = currentTileID % 5;
            int row = currentTileID / 5;
            selectedTileBox.setPosition(10 + col * (tileSize + 2), 10 + row * (tileSize + 2));
        }
        else {
            // Altrimenti → Zoom plastico
            float zoomFactor = 1.0f + (-event.mouseWheelScroll.delta * 0.1f);
            zoomFactor = std::clamp(zoomFactor, 0.8f, 1.2f); // limita per evitare zoom eccessivi

            game.adjustZoom(zoomFactor);
        }
    }


    if (event.type == sf::Event::MouseButtonPressed && !ImGui::GetIO().WantCaptureMouse) {
        if (event.mouseButton.button == sf::Mouse::Left) {

            // Selezione attiva? Se sì, cancella!
            if (selectionActive) {
                selectionActive = false;
                selectionStart = { -1, -1 };
                selectionEnd = { -1, -1 };
            }
            //////////////////////////////////////

            if (currentTool == DrawTool::Event) {
                selectedEntity = hoveredEntity;
                propertyPanel.setSelectedEntity(selectedEntity);
            }

            if (currentTool != DrawTool::Event) {
                selectedEntity = {}; // resetta la selezione se cambi tool
            }


            switch (currentTool) {
            case DrawTool::Brush: {
                TileInstance tile;
                tile.tilesetName = tilemap->getCurrentTileset();
                tile.tileID = currentTileID;

                if (!tile.isValid() || !tileSetManager->hasTileset(tile.tilesetName)) {
                    if (tile.isValid()) {
                        //std::cerr << "[CRASH PREVENT] Tileset mancante: " << tile.tilesetName << "\n";
                    }
                    // Se tile vuoto prosegui
                }

                pushUndoState();
                tilemap->setTile(currentLayerIndex, x, y, tile);
                break;
            }
            case DrawTool::Fill: {
                TileInstance target = tilemap->getTile(currentLayerIndex, x, y);
                TileInstance replacement;
                replacement.tilesetName = currentTilesetName;
                replacement.tileID = currentTileID;
                tilemap->floodFill(currentLayerIndex, x, y, target, replacement);
                break;
            }
            case DrawTool::Rectangle:
                rectangleStart = { x, y };
                break;

            // Eventi
            case DrawTool::Event:
                handleEventSelection(x, y); // 👈 funzione che ora creeremo
                break;
            }
           

        }
        else if (event.mouseButton.button == sf::Mouse::Right) {
            tilemap->clearTile(currentLayerIndex, x, y);

        }
        else if (event.mouseButton.button == sf::Mouse::Middle) {
            //std::cout << "[gameState] pulsante centrale premuto" << std::endl;
            isSelectingArea = true;
            selectionActive = true; // <<<<<<<<<<<<< AGGIUNGI QUESTA
            selectionStart = { x, y };
            selectionEnd = { x, y };
        }


    }


    if (event.type == sf::Event::MouseMoved && isSelectingArea) {
        sf::Vector2i pixelPos = sf::Mouse::getPosition(game.getWindow());
        sf::Vector2f worldPos = game.getWindow().mapPixelToCoords(pixelPos, game.getGameView());
        selectionEnd = sf::Vector2i(static_cast<int>(worldPos.x / tileSize), static_cast<int>(worldPos.y / tileSize));
    }



    if (event.type == sf::Event::MouseButtonReleased) {
        if (event.mouseButton.button == sf::Mouse::Left) {
            if (currentTool == DrawTool::Rectangle && rectangleStart.x >= 0 && rectangleStart.y >= 0) {
                sf::Vector2i pixelPos = sf::Mouse::getPosition(game.getWindow());
                sf::Vector2f worldPos = game.getWindow().mapPixelToCoords(pixelPos, game.getGameView());
                int endX = static_cast<int>(worldPos.x / tileSize);
                int endY = static_cast<int>(worldPos.y / tileSize);

                for (int y = std::min(rectangleStart.y, endY); y <= std::max(rectangleStart.y, endY); ++y) {
                    for (int x = std::min(rectangleStart.x, endX); x <= std::max(rectangleStart.x, endX); ++x) {
                        TileInstance tile;
                        tile.tilesetName = tilemap->getCurrentTileset();
                        tile.tileID = currentTileID;

                        pushUndoState();
                        tilemap->setTile(currentLayerIndex, x, y, tile);
                    }
                }
                rectangleStart = { -1, -1 };
            }
        }
        else if (event.mouseButton.button == sf::Mouse::Middle) {
            if (isSelectingArea) {
                isSelectingArea = false;
                selectionActive = true; //la selezione rimane visibile
                selectionEnd = sf::Vector2i(x, y); // confermiamo il punto finale
            }
            
        }
    }

    if (event.type == sf::Event::MouseMoved && isSelectingArea) {
        selectionEnd = { x, y };
    }
}

void EditorState::handleEventSelection(int x, int y) {
    // Per ora facciamo solo debug
    std::cout << "[Event Selection] Cursore su (" << x << ", " << y << ")\n";

    // Qui dopo collegheremo:
    // - TileInstance tile = tilemap->getTile(currentLayer, x, y);
    // - PlacedAnimationGroup gruppi se presenti
    // - Mostrare nel pannello
}


void EditorState::deleteTileAndGroupAt(int x, int y) {
    if (!tilemap) return;
    if (x < 0 || x >= mapWidth || y < 0 || y >= mapHeight) return;

    // 1. Cancella il tile normale
    tilemap->clearTile(currentLayerIndex, x, y);

    // 2. Cancella eventuale gruppo animato piazzato lì
    placedGroups.erase(
        std::remove_if(placedGroups.begin(), placedGroups.end(), [=](const PlacedAnimationGroup& g) {
            return g.position == sf::Vector2i(x, y) && g.layer == currentLayerIndex;
            }),
        placedGroups.end()
    );
}


void EditorState::deleteTilesAndGroupsInSelection() {
    int minX = std::min(selectionStart.x, selectionEnd.x);
    int minY = std::min(selectionStart.y, selectionEnd.y);
    int maxX = std::max(selectionStart.x, selectionEnd.x);
    int maxY = std::max(selectionStart.y, selectionEnd.y);

    for (int y = minY; y <= maxY; ++y) {
        for (int x = minX; x <= maxX; ++x) {
            deleteTileAndGroupAt(x, y);
        }
    }
}



void EditorState::updateBrushing() {
    if (!tilemap || !tileSetManager || !tileSetManager->hasTileset(currentTilesetName)) {
        // Impedisci interazione finché i tileset non sono pronti
        std::cerr << "[DEBUG] Interazione disattivata: tilemap o tileset non ancora pronti.\n";
        return;
    }

    if (currentTool != DrawTool::Brush) return;
    if (!sf::Mouse::isButtonPressed(sf::Mouse::Left)) return;
    if (ImGui::GetIO().WantCaptureMouse) return;

    sf::Vector2i pixelPos = sf::Mouse::getPosition(game.getWindow());
    sf::Vector2f worldPos = game.getWindow().mapPixelToCoords(pixelPos, game.getGameView());
    int x = static_cast<int>(worldPos.x / tileSize);
    int y = static_cast<int>(worldPos.y / tileSize);

    TileInstance tile;
    tile.tilesetName = tilemap->getCurrentTileset(); // 

    tile.tileID = currentTileID;

    pushUndoState();
    tilemap->setTile(currentLayerIndex, x, y, tile);


}


void EditorState::update(float dt) {
    //std::cout << "[EditorState::update] Inizio update()\n";

    //selectionFadeTimer += dt; // << aggiungi subito questo!


    try {

        if (selectionActive) {
            selectionFadeTimer += dt;
            if (selectionFadeTimer > 2 * 3.14159265f) { // 2π
                selectionFadeTimer -= 2 * 3.14159265f; // evita overflow del timer
            }
        }

        // Qui non setti niente a mano, perché lo fa già updateHoverSelection() ora!
        // Devi solo fare:
        propertyPanel.update();
        //updateHoverSelection(); // << aggiungi questo se non l'hai ancora messo

        //std::cout << "[EditorState::update] Fatto propertyPanel.update()\n";

        //std::cout << "[EditorState::update] Chiamo animationPanel.update()\n";
        animationPanel.update();
        //std::cout << "[EditorState::update] Fatto animationPanel.update()\n";

        //std::cout << "[EditorState::update] Chiamo animationManager.updateAll()\n";
        animationManager.updateAll(dt);
        //std::cout << "[EditorState::update] Fatto animationManager.updateAll()\n";
        //updateBrushing();
        handleEdgeScrolling(dt);

        //std::cout << "[EditorState::update] Fine update()\n";

        if (tilesetEditor)
            tilemap->setCurrentTileset(tilesetEditor->getSelectedTilesetName());

        updateHoverSelection();
    }
    catch (const std::exception& e) {
        std::cerr << "[EditorState::update] ECCEZIONE: " << e.what() << "\n";
    }
    catch (...) {
        std::cerr << "[EditorState::update] ECCEZIONE GENERICA\n";
    }

    //std::cout << "[DEBUG] update finito\n";
}





void EditorState::render() {
    sf::RenderWindow& window = game.getWindow();

    int currentTile = tileSelectorPanel.getSelectedTileID();

    // Calcolo posizione mouse e tile
    sf::Vector2i mousePixel = sf::Mouse::getPosition(window);
    sf::Vector2f worldPos = window.mapPixelToCoords(mousePixel, game.getGameView());
    int tileX = static_cast<int>(worldPos.x / tileSize);
    int tileY = static_cast<int>(worldPos.y / tileSize);
    std::string mapFileName = fs::path(configPath).filename().string();

    // --- 1. Disegna la mappa con la vera gameView ---
    window.setView(game.getGameView());
    if (tilemap && tileSetManager && tileSetManager->hasTileset(currentTilesetName)) {
        window.draw(*tilemap);
    }

    //window.draw(*tilemap);

    // --- 2. Disegna gruppi animati
    for (const auto& placed : placedGroups) {
        for (const auto& group : groupEditor->getGroups()) {
            if (group.name == placed.groupName) {
                int frameIndex = static_cast<int>((ImGui::GetTime() / group.frameDuration)) % group.frames.size();
                const auto& frame = group.frames[frameIndex];

                const sf::Texture* texture = tileSetManager->getTexture(currentTilesetName);
                if (!texture) continue;

                int columns = texture->getSize().x / tileSize;

                for (const auto& tile : frame) {
                    int tx = (tile.tileID % columns) * tileSize;
                    int ty = (tile.tileID / columns) * tileSize;

                    sf::Sprite sprite;
                    sprite.setTexture(*texture);
                    sprite.setTextureRect(sf::IntRect(tx, ty, tileSize, tileSize));
                    sprite.setPosition((placed.position.x + tile.offset.x) * tileSize,
                        (placed.position.y + tile.offset.y) * tileSize);
                    window.draw(sprite);
                }
            }
        }
    }

    if (placingGroup && sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
        sf::Vector2i mousePixel = sf::Mouse::getPosition(game.getWindow());
        sf::Vector2f worldPos = game.getWindow().mapPixelToCoords(mousePixel, game.getGameView());
        sf::Vector2i tilePos(
            static_cast<int>(worldPos.x / tileSize),
            static_cast<int>(worldPos.y / tileSize)
        );

        for (const auto& group : groupEditor->getGroups()) {
            if (group.name == currentGroupToPlace) {
                placedGroups.push_back({ group.name, tilePos, currentLayerIndex });
                tilemap->addAnimatedGroup(group, tilePos);
                break;
            }
        }

        placingGroup = false;
    }


    // - 3.  Griglia opzionale
    if (showGrid) {
        sf::VertexArray gridLines(sf::Lines);

        for (int y = 0; y < mapHeight; ++y) {
            for (int x = 0; x < mapWidth; ++x) {
                bool isEmpty = true;

                // Controlla tutti i layer
                for (int layer = 0; layer < tilemap->getLayerCount(); ++layer) {
                    if (tilemap->getTile(layer, x, y).isValid()) {
                        isEmpty = false;
                        break;
                    }
                }

                if (isEmpty) {
                    sf::Color color = sf::Color(255, 0, 0, 120); // rosso trasparente
                    gridLines.append(sf::Vertex(sf::Vector2f(x * tileSize, y * tileSize), color));
                    gridLines.append(sf::Vertex(sf::Vector2f((x + 1) * tileSize, y * tileSize), color));
                    gridLines.append(sf::Vertex(sf::Vector2f((x + 1) * tileSize, (y + 1) * tileSize), color));
                    gridLines.append(sf::Vertex(sf::Vector2f((x + 1) * tileSize, (y + 1) * tileSize), color));
                    gridLines.append(sf::Vertex(sf::Vector2f(x * tileSize, (y + 1) * tileSize), color));
                    gridLines.append(sf::Vertex(sf::Vector2f(x * tileSize, y * tileSize), color));
                }
            }
        }







        window.draw(gridLines);
    }
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // 1. Hover e selezione
    SelectedEntity entityToDraw = (selectedEntity.type != SelectedEntity::Type::None) ? selectedEntity : hoveredEntity;

    if (entityToDraw.type != SelectedEntity::Type::None) {
        sf::RectangleShape highlight = getHighlightRectangle(entityToDraw);
        highlight.setFillColor(sf::Color(100, 200, 255, 80));
        highlight.setOutlineColor(sf::Color(100, 200, 255, 180));
        highlight.setOutlineThickness(2.f);
        window.draw(highlight);
    }
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    //--- 4. ORA Disegna la selezione se attiva (FUORI DALLA GRIGLIA)
    if (selectionActive) {
        float alpha = 100 + std::sin(selectionFadeTimer * 2.f) * 50;
        alpha = std::clamp(alpha, 30.f, 180.f);

        sf::RectangleShape selectionRect;
        selectionRect.setFillColor(sf::Color(255, 255, 255, static_cast<sf::Uint8>(alpha)));
        selectionRect.setOutlineColor(sf::Color(255, 0, 0, 200));
        selectionRect.setOutlineThickness(2.f);

        int minX = std::min(selectionStart.x, selectionEnd.x);
        int minY = std::min(selectionStart.y, selectionEnd.y);
        int maxX = std::max(selectionStart.x, selectionEnd.x);
        int maxY = std::max(selectionStart.y, selectionEnd.y);

        selectionRect.setPosition(minX * tileSize, minY * tileSize);
        selectionRect.setSize(sf::Vector2f((maxX - minX + 1) * tileSize, (maxY - minY + 1) * tileSize));

        // Disegna alone glow PRIMA
        for (int i = 0; i < 5; ++i) {
            sf::RectangleShape glow = selectionRect;
            glow.setFillColor(sf::Color::Transparent);
            glow.setOutlineThickness(6.f + i * 2.f);
            glow.setOutlineColor(sf::Color(255, 0, 0, 50 - i * 8));
            window.draw(glow);
        }

        // Disegna rettangolo finale della selezione
        window.draw(selectionRect);
    }


   


    window.setView(window.getDefaultView());
    // Dopo 

    if (!ImGui::GetIO().WantCaptureMouse) {
        sf::Vector2i pixelPos = sf::Mouse::getPosition(window);
        sf::Vector2f worldPos = window.mapPixelToCoords(pixelPos, game.getGameView());

        sf::Text layerInfo;
        layerInfo.setFont(font);
        layerInfo.setCharacterSize(14);
        layerInfo.setFillColor(sf::Color::White);

        const char* layerNames[] = { "Base", "Decor", "Entity", "Top" };
        sf::Color layerColor = sf::Color::White;

        switch (currentLayerIndex) {
        case BASE_LAYER: layerColor = sf::Color::Green; break;
        case DECOR_LAYER: layerColor = sf::Color(138, 43, 226); break;
        case ENTITY_LAYER: layerColor = sf::Color::Blue; break;
        case TOP_LAYER: layerColor = sf::Color::Red; break;
        }

        layerInfo.setString("Layer: " + std::string(layerNames[currentLayerIndex]));
        layerInfo.setFillColor(layerColor);
        layerInfo.setPosition(worldPos.x + 16, worldPos.y - 20);
        window.draw(layerInfo);
    }


    // --- Barra dei menu ---
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Proprieta' Mondo...")) showWorldSettings = true;

            if (ImGui::MenuItem("Salva mappa", "Ctrl+S")) {
                saveFullMap("Mappa");
            }

            if (ImGui::MenuItem("Carica mappa", "Ctrl+L")) {
                loadFullMap("Mappa");
            }


            if (ImGui::MenuItem("Esci")) {
                game.changeState(std::make_unique<MenuState>(game));
            }
            ImGui::EndMenu();
        }


        if (ImGui::BeginMenu("Strumenti")) {
            if (ImGui::MenuItem("Gestione Tileset...")) {
                showTilesetEditor = true;
            }

            if (ImGui::MenuItem("Editor Gruppi Animati...")) {
                showGroupEditor = true;
            }

            if (ImGui::MenuItem("Editor Eventi...")) {
                showEventEditor = true;
            }


            if (ImGui::MenuItem("Script Visuale NodeGraph")) {
                openNodeGraph = true;
            }

            if (ImGui::MenuItem("Player", nullptr, showPlayerPanel)) {
                showPlayerPanel = !showPlayerPanel;
            }

            if (ImGui::MenuItem("Griglia...")) {
                ImGui::MenuItem("Mostra griglia", nullptr, &showGrid);
                showGrid = true;
            }

            
            ImGui::EndMenu();
        }

       
            
        



        // Info in tempo reale nella barra
        ImGui::Separator();
        const char* layerNames[] = { "Base", "Decor", "Entity", "Top" };

        ImGui::Text("Tile ID=%d | X=%d | Y=%d | Layer: %s | Mappa: %s | %dx%d | Zoom: %.2fx",
            currentTileID, tileX, tileY, layerNames[currentLayerIndex], mapFileName.c_str(), mapWidth, mapHeight, currentZoom);


        ImGui::EndMainMenuBar();
    }


    if (openNodeGraph)
    {
        //ImGui::Begin("Editor NodeGraph", &openNodeGraph);

        //if (ImGui::Button("Avvia Flow"))
        //{
        //    nodeGraph.startFlow();
        //}

        nodeGraph.draw();

        //ImGui::End();
    }


    // --- Finestra Proprietà Mondo ---
    if (showWorldSettings && worldSettings) {
        ImGui::SetNextWindowPos(ImVec2(320, 34), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(300, 320), ImGuiCond_FirstUseEver);
        worldSettings->renderPanel(&showWorldSettings);
    }

    if (showTilesetEditor && tilesetEditor) {
        tilesetEditor->render(&showTilesetEditor);
    }

    if (showGroupEditor && groupEditor) {
        groupEditor->setTileSize(tileSize, tileSize); // utile nel caso cambi dinamicamente
        groupEditor->setCurrentTileID(currentTileID); // per sapere cosa piazzare
        groupEditor->setTilesetName(currentTilesetName); // aggiornato ogni frame
        groupEditor->render(&showGroupEditor, placingGroup, currentGroupToPlace);

    }

    if (showEventEditor && eventEditorPanel) {
        eventEditorPanel->render(&showEventEditor);   // passa il flag così la finestra può chiudersi da sola
    }

    if (playerPanel) {
        playerPanel->render(nodeGraph.nodes);
    }




    // Finestra Layer Attivo
    ImGui::SetNextWindowPos(ImVec2(10, 34), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(300, 600), ImGuiCond_FirstUseEver);
    ImGui::Begin("Strumenti & Layer");

    const char* layerNames[] = { "Base", "Decor", "Entity", "Top" };
    ImGui::Text("Layer corrente:");
    ImGui::Combo("##Layer", &currentLayerIndex, layerNames, IM_ARRAYSIZE(layerNames));

    if (ImGui::Button("Aggiungi Layer")) {
        tilemap->addLayer();
    }

    ImGui::Separator();
    ImGui::Text("Strumenti:");

    if (ImGui::RadioButton("Pennello", currentTool == DrawTool::Brush)) currentTool = DrawTool::Brush;
    if (ImGui::RadioButton("Secchiello", currentTool == DrawTool::Fill)) currentTool = DrawTool::Fill;
    if (ImGui::RadioButton("Rettangolo", currentTool == DrawTool::Rectangle)) currentTool = DrawTool::Rectangle;
    if (ImGui::RadioButton("Eventi", currentTool == DrawTool::Event)) currentTool = DrawTool::Event;

    ImGui::End();



  

    // --- Finestra Animazioni ---
    ImGui::SetNextWindowPos(ImVec2(game.getWindow().getSize().x - 310, 34), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(300, 250), ImGuiCond_FirstUseEver);
    ImGui::Begin("Gestione Animazioni");
    animationPanel.render();
    ImGui::End();

    // --- Finestra Proprietà Entità ---
    ImGui::SetNextWindowPos(ImVec2(game.getWindow().getSize().x - 310, 300), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_FirstUseEver);
    //ImGui::Begin("Proprieta Entita");
    propertyPanel.renderImGui();
    //ImGui::End();

    // --- Avviso mancanza metadata ---
    if (tileMetadata.empty()) {
        ImGui::Begin("Errore");
        ImGui::Text("Nessun metadata caricato!");
        ImGui::End();
    }

    if (placingGroup && sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
        sf::Vector2i mousePixel = sf::Mouse::getPosition(game.getWindow());
        sf::Vector2f worldPos = game.getWindow().mapPixelToCoords(mousePixel, game.getGameView());
        sf::Vector2i tilePos(
            static_cast<int>(worldPos.x / tileSize),
            static_cast<int>(worldPos.y / tileSize)
        );


        for (const auto& group : groupEditor->getGroups()) {
            if (group.name == currentGroupToPlace) {
                placedGroups.push_back({ group.name, tilePos, currentLayerIndex }); // disegno subito
                tilemap->addAnimatedGroup(group, tilePos);                         // salvo nei dati veri
                break;
            }
        }

        placingGroup = false;

    }


    //std::cout << "[DEBUG] draw finito\n";
}


sf::Color EditorState::getBackgroundColor() const {
    return config.backgroundColor;
}



void EditorState::salvaTileMetadata(const std::string& path) {
    json j;
    for (const auto& [id, info] : tileMetadata) {
        j[std::to_string(id)] = {
            { "name", info.name },
            { "type", info.type },
            { "walkable", info.walkable },
            { "category", info.category },
            { "genre", info.genre },
             { "season", info.season }
        };
    }
    std::ofstream out(path);
    out << std::setw(4) << j;
    std::cout << "[Editor] Salvato metadata in " << path << "\n";
}

void EditorState::loadTileMetadata(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "[Editor] Impossibile aprire il file metadata: " << path << std::endl;
        return;
    }

    try {
        json j;
        file >> j;

        tileMetadata.clear();

        for (auto it = j.begin(); it != j.end(); ++it) {
            int id = std::stoi(it.key());
            const json& value = it.value();

            TileInfo info;
            info.name = value.value("name", "Sconosciuto");
            info.type = value.value("type", "Unknown");
            info.walkable = value.value("walkable", true);
            info.category = value.value("category", "Generico");
            info.genre = value.value("genre", "Nessuno");
            info.season = value.value("season", "all");
            tileMetadata[id] = info;
        }

        std::cout << "[Editor] Caricati " << tileMetadata.size() << " metadati tile.\n";
    }
    catch (const std::exception& e) {
        std::cerr << "[Editor] Errore nel parsing dei metadati: " << e.what() << std::endl;
    }
}



void EditorState::saveMap(const std::string& path) {
    std::ofstream out(path);
    if (!out.is_open()) {
        std::cerr << "Impossibile aprire il file per salvare la mappa: " << path << "\n";
        return;
    }

    int layerCount = tilemap->getLayerCount();
    out << layerCount << "\n"; // Salva il numero di layer

    for (int layer = 0; layer < layerCount; ++layer) {
        for (int y = 0; y < mapHeight; ++y) {
            for (int x = 0; x < mapWidth; ++x) {
                TileInstance tile = tilemap->getTile(layer, x, y);
                if (tile.isValid())
                    out << tile.tilesetName << ":" << tile.tileID << ' ';
                else
                    out << "null ";
            }
            out << '\n';
        }
    }

    // Salva anche gli eventi nella stessa cartella
    std::filesystem::path basePath = std::filesystem::path(path).parent_path();
    eventSystem.saveToFile((basePath / "map.events.json").string());

    std::cout << "[Editor] Mappa salvata in " << path << "\n";

    savePlacedGroups("assets/placed_groups.json"); // Salva anche i gruppi animati
}


void EditorState::loadMap(const std::string& path) {
    std::ifstream in(path);
    if (!in.is_open()) {
        std::cerr << "[Editor] Errore nel caricamento della mappa: " << path << "\n";
        return;
    }

    int layerCount = 0;
    in >> layerCount; // Legge il numero di layer
    if (layerCount <= 0) {
        std::cerr << "[Editor] Numero di layer non valido!\n";
        return;
    }

    tilemap->clear();
    for (int i = 0; i < layerCount; ++i) {
        tilemap->addLayer();
    }

    for (int layer = 0; layer < layerCount; ++layer) {
        for (int y = 0; y < mapHeight; ++y) {
            for (int x = 0; x < mapWidth; ++x) {
                std::string cell;
                in >> cell;
                if (cell == "null") continue;

                size_t delim = cell.find(':');
                if (delim != std::string::npos) {
                    std::string tileset = cell.substr(0, delim);
                    int id = std::stoi(cell.substr(delim + 1));
                    TileInstance tile{ tileset, id };
                    tilemap->setTile(layer, x, y, tile);
                }
            }
        }
    }

    std::cout << "[Editor] Mappa caricata da " << path << "\n";

    loadPlacedGroups("assets/placed_groups.json"); // Carica anche i gruppi

    autoLoadTilesetsIfMissing();

}



void EditorState::loadAvailableTilesets(const std::string& folder) {
    tilesetPaths.clear();

    try {
        for (const auto& entry : fs::directory_iterator(folder)) {
            if (!entry.is_regular_file()) continue;

            std::string ext = entry.path().extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            if (ext != ".png") continue;

            tilesetPaths.push_back(entry.path().string());
            std::cout << "[Editor] PNG tileset trovato: " << entry.path().string() << "\n";
        }

        if (!tilesetPaths.empty()) {
            currentTilesetIndex = 0;
            loadTilesetFromFolder(tilesetPaths[0]); // usa direttamente percorso png
        }
        else {
            std::cerr << "[Editor] Nessun PNG trovato in: " << folder << "\n";
        }
    }
    catch (const std::exception& e) {
        std::cerr << "[Editor] Errore durante la scansione PNG: " << e.what() << "\n";
    }
}




void EditorState::handleTilePlacement(sf::Event& event) {
    if (event.type != sf::Event::MouseButtonPressed || event.mouseButton.button != sf::Mouse::Left)
        return;

    sf::Vector2i pixelPos = sf::Mouse::getPosition(game.getWindow());
    sf::Vector2f worldPos = game.getWindow().mapPixelToCoords(pixelPos, game.getGameView());

    int x = static_cast<int>(worldPos.x) / tileSize;
    int y = static_cast<int>(worldPos.y) / tileSize;

    TileInstance tile;
    tile.tilesetName = tilemap->getCurrentTileset(); // 

    tile.tileID = currentTileID;
    tilemap->setTile(currentLayerIndex, x, y, tile);


    //std::cout << "[Editor] Piazzato tile " << currentTileID << " in (" << x << ", " << y << ")\n";
}


void EditorState::renderTilesetView() {
    // Finestra scrollabile
    ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(0, 0, 0, 0));  // bg trasparente

    ImGui::BeginChild("ViewTileset", ImVec2(300, 300), false,
                      ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_AlwaysVerticalScrollbar);

    ImGui::Text("Tileset:");
    ImGui::Separator();

    float displaySize = static_cast<float>(tileScreenSize);

    int spacing = 2;

    int texCols = tileSelectorTexture.getSize().x / tileSize;
    int texRows = tileSelectorTexture.getSize().y / tileSize;

    // Spazio totale da scrollare
    ImVec2 contentSize = ImVec2(texCols * (displaySize + spacing), texRows * (displaySize + spacing));
    
    ImVec2 origin = ImGui::GetCursorScreenPos();
    ImGui::Dummy(contentSize);

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    

    ImTextureID texID = reinterpret_cast<ImTextureID>(static_cast<uintptr_t>(tileSelectorTexture.getNativeHandle()));

    for (int i = 0; i < maxTileID; ++i) {
        int col = i % texCols;
        int row = i / texCols;

        ImVec2 pos = ImVec2(origin.x + col * (displaySize + spacing), origin.y + row * (displaySize + spacing));
        ImVec2 size = ImVec2(displaySize, displaySize);

        float tx = (i % texCols) * tileSize;
        float ty = (i / texCols) * tileSize;

        ImVec2 uv0 = ImVec2(tx / (float)tileSelectorTexture.getSize().x, ty / (float)tileSelectorTexture.getSize().y);
        ImVec2 uv1 = ImVec2((tx + tileSize) / (float)tileSelectorTexture.getSize().x, (ty + tileSize) / (float)tileSelectorTexture.getSize().y);

        // 1. Sfondo checker trasparente tipo GIMP (facoltativo)
        drawList->AddImage(texID, pos, ImVec2(pos.x + size.x, pos.y + size.y), uv0, uv1);


        // 2. Disegna tile sopra (con alpha)
        //drawList->AddImage(texID, pos, ImVec2(pos.x + size.x, pos.y + size.y), uv0, uv1);


        ImRect tileRect(pos, ImVec2(pos.x + size.x, pos.y + size.y));
        if (ImGui::IsMouseHoveringRect(tileRect.Min, tileRect.Max) && ImGui::IsMouseClicked(0)) {
            currentTileID = i;
            std::cout << "[Tileset View] Tile selezionato: " << i << "\n";
        }

        if (i == currentTileID) {
            drawList->AddRect(pos, ImVec2(pos.x + size.x, pos.y + size.y), IM_COL32(255, 0, 0, 255), 0.f, 0, 2.f);
        }
    }


    ImGui::EndChild();

    ImGui::PopStyleColor();


    ImGui::SameLine();
    ImGui::BeginGroup();
    ImGui::Text("Strumenti:");

    if (ImGui::RadioButton("Pennello", currentTool == DrawTool::Brush)) currentTool = DrawTool::Brush;
    if (ImGui::RadioButton("Secchiello", currentTool == DrawTool::Fill)) currentTool = DrawTool::Fill;
    if (ImGui::RadioButton("Rettangolo", currentTool == DrawTool::Rectangle)) currentTool = DrawTool::Rectangle;
    if (ImGui::RadioButton("Eventi", currentTool == DrawTool::Event)) currentTool = DrawTool::Event;

    //if (ImGui::RadioButton("Seleziona", currentTool == DrawTool::Selector)) currentTool = DrawTool::Selector;

    ImGui::EndGroup();

}

void EditorState::drawTransparentBackground(ImDrawList* drawList, ImVec2 pos, float size, float squareSize) {
    for (float y = 0; y < size; y += squareSize) {
        for (float x = 0; x < size; x += squareSize) {
            bool light = static_cast<int>((x / squareSize + y / squareSize)) % 2 == 0;
            ImU32 color = light ? IM_COL32(220, 220, 220, 255) : IM_COL32(180, 180, 180, 255);
            ImVec2 p1 = ImVec2(pos.x + x, pos.y + y);
            ImVec2 p2 = ImVec2(pos.x + x + squareSize, pos.y + y + squareSize);
            drawList->AddRectFilled(p1, p2, color);
        }
    }
}


void EditorState::handleEdgeScrolling(float dt) {
    const float baseSpeed = 500.f;
    float speed = baseSpeed * dt;

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift) || sf::Keyboard::isKeyPressed(sf::Keyboard::RShift)) {
        speed *= 2.f; // Turbo mode
    }

    sf::RenderWindow& window = game.getWindow();
    sf::Vector2i mousePos = sf::Mouse::getPosition(window);
    sf::Vector2u windowSize = window.getSize();
    const int margin = 20;

    sf::View view = game.getGameView();
    sf::Vector2f center = view.getCenter();

    // Verifica se il mouse è DENTRO la finestra
    bool mouseInsideWindow = mousePos.x >= 0 && mousePos.x < static_cast<int>(windowSize.x) &&
        mousePos.y >= 0 && mousePos.y < static_cast<int>(windowSize.y);

    bool mouseOverImGui = ImGui::GetIO().WantCaptureMouse;
    bool mouseOverMenuBar = (mousePos.y >= 0 && mousePos.y <= static_cast<int>(ImGui::GetFrameHeight()));

    // Scroll col mouse SOLO se:
    // 1. Mouse è dentro la finestra
    // 2. Non stai interagendo con ImGui
    // 3. Non sei sopra la menu bar
    if (mouseInsideWindow && !mouseOverImGui && !mouseOverMenuBar) {
        if (mousePos.x <= margin) center.x -= speed;
        if (mousePos.x >= static_cast<int>(windowSize.x) - margin) center.x += speed;
        if (mousePos.y <= margin) center.y -= speed;
        if (mousePos.y >= static_cast<int>(windowSize.y) - margin) center.y += speed;
    }

    // Scroll coi tasti SEMPRE abilitato
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) center.x -= speed;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) center.x += speed;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) center.y -= speed;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) center.y += speed;

    view.setCenter(center);
    game.setGameView(view); // Salva la nuova posizione della view
    window.setView(view);   // Applica la nuova view
}

void EditorState::generaMappaAutomatica(const std::string& stagione) {
    for (int y = 0; y < mapHeight; ++y) {
        for (int x = 0; x < mapWidth; ++x) {
            std::vector<int> validTiles;
            for (const auto& [id, info] : tileMetadata) {
                if (info.season == stagione || info.season == "all") {
                    validTiles.push_back(id);
                }
            }
            if (!validTiles.empty()) {
                int randomIndex = rand() % validTiles.size();
                int selected = validTiles[randomIndex];
                TileInstance tile;
                tile.tilesetName = tilemap->getCurrentTileset(); // 

                tile.tileID = currentTileID;
                tilemap->setTile(currentLayerIndex, x, y, tile);


            }
        }
    }
}

// Nuova funzione: pannello per modificare world.ini
void EditorState::renderWorldConfigPanel() {
    static WorldConfig tempConfig = loadWorldConfig("assets/world.ini");

    static char tilesetPath[256] = "";
    static char metadataPath[256] = "";

    if (tilesetPath[0] == '\0') strncpy_s(tilesetPath, sizeof(tilesetPath), tempConfig.tilesetPath.c_str(), _TRUNCATE);
    if (metadataPath[0] == '\0') strncpy_s(metadataPath, sizeof(metadataPath), tempConfig.metadataPath.c_str(), _TRUNCATE);


    ImGui::Begin("Proprietà del mondo", &showWorldConfigPanel);

    float widthKm = tempConfig.mapWidth * tempConfig.tileWidth / 1000.0f;
    float heightKm = tempConfig.mapHeight * tempConfig.tileHeight / 1000.0f;

    ImGui::Text("Dimensione del mondo:");
    ImGui::SliderFloat("Larghezza (km)", &widthKm, 1.0f, 100.0f);
    ImGui::SliderFloat("Altezza (km)", &heightKm, 1.0f, 100.0f);

    ImGui::InputInt("Larghezza tile (px)", &tempConfig.tileWidth);
    ImGui::InputInt("Altezza tile (px)", &tempConfig.tileHeight);
    ImGui::InputInt("Dimensione su schermo (px)", &tempConfig.tileScreenSize);

    ImGui::InputText("Tileset Path", tilesetPath, IM_ARRAYSIZE(tilesetPath));
    ImGui::InputText("Metadata Path", metadataPath, IM_ARRAYSIZE(metadataPath));

    if (ImGui::Button("Salva")) {
        std::ofstream out("assets/world.ini");
        out << "[World]\n";
        out << "tilewidth =" << tempConfig.tileWidth << "\n";
        out << "tileheight = " << tempConfig.tileHeight << "\n";
        out << "mapwidth = " << static_cast<int>(widthKm * 1000 / tempConfig.tileWidth) << "\n";
        out << "mapheight = " << static_cast<int>(heightKm * 1000 / tempConfig.tileHeight) << "\n";
        out << "tilesetpath = " << tilesetPath << "\n";
        out << "metadatapath = " << metadataPath << "\n";
        out << "tileScreenSize = " << tempConfig.tileScreenSize << "\n";
        out.close();
        std::cout << "[Editor] world.ini salvato\n";
    }

    ImGui::End();
}


// Aggiunta per aggiornare GUI con lista tileset attivi
void EditorState::renderTilesetDropdown() {
    if (ImGui::BeginCombo("Tileset Attivo", currentTilesetName.c_str())) {
        for (const auto& name : tileSetManager->getLoadedTilesetNames()) {
            bool selected = (currentTilesetName == name);
            if (ImGui::Selectable(name.c_str(), selected)) {
                currentTilesetName = name;
                tilemap->setCurrentTileset(name);
                std::cout << "[Editor] Tileset attivo cambiato: " << name << "\n";
            }
            if (selected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
}

void EditorState::savePlacedGroups(const std::string& path) {
    nlohmann::json j;

    for (const auto& placed : placedGroups) {
        j.push_back({
            {"groupName", placed.groupName},
            {"position", { placed.position.x, placed.position.y }},
            {"layer", placed.layer}
            });
    }

    std::ofstream out(path);
    if (out.is_open()) {
        out << std::setw(4) << j;
        std::cout << "[Editor] PlacedGroups salvati in " << path << "\n";
    }
    else {
        std::cerr << "[Editor] Errore salvataggio PlacedGroups!\n";
    }
}

void EditorState::loadPlacedGroups(const std::string& path) {
    std::ifstream in(path);
    if (!in.is_open()) {
        std::cerr << "[Editor] File placed_groups.json non trovato.\n";
        return;
    }

    nlohmann::json j;
    in >> j;

    placedGroups.clear(); // pulisci i gruppi esistenti!

    for (const auto& item : j) {
        PlacedAnimationGroup placed;
        placed.groupName = item.value("groupName", "");
        auto pos = item["position"];
        placed.position = sf::Vector2i(pos[0], pos[1]);
        placed.layer = item.value("layer", BASE_LAYER);

        placedGroups.push_back(placed);

        // Inoltre ricrea anche i gruppi animati sulla TileMap!
        for (const auto& group : groupEditor->getGroups()) {
            if (group.name == placed.groupName) {
                tilemap->addAnimatedGroup(group, placed.position);
                break;
            }
        }
    }

    std::cout << "[Editor] PlacedGroups caricati da " << path << "\n";
}

void EditorState::autoLoadTilesetsIfMissing() {
    std::unordered_set<std::string> requiredTilesets;

    // 1. Scansiona tutti i tile in tutti i layer
    int layerCount = tilemap->getLayerCount();
    for (int layer = 0; layer < layerCount; ++layer) {
        for (int y = 0; y < mapHeight; ++y) {
            for (int x = 0; x < mapWidth; ++x) {
                TileInstance tile = tilemap->getTile(layer, x, y);
                if (tile.isValid()) {
                    requiredTilesets.insert(tile.tilesetName);
                }
            }
        }
    }

    // 2. Carica quelli che mancano
    for (const auto& name : requiredTilesets) {
        if (!tileSetManager->hasTileset(name)) {
            std::string path = "assets/stagioni/" + name + ".png";
            if (std::filesystem::exists(path)) {
                tileSetManager->loadTileset(name, path);
                std::cout << "[Editor] Tileset '" << name << "' caricato automaticamente!\n";
            }
            else {
                std::cerr << "[Editor] Tileset '" << name << "' non trovato!\n";
            }
        }
    }
}

void EditorState::pushUndoState() {
    if (tilemap) {
        undoStack.push_back(tilemap->getFullMapState()); // Ok, full map state
        if (undoStack.size() > 50) {
            undoStack.erase(undoStack.begin()); // Rimuove il più vecchio se superiamo il limite
        }
        redoStack.clear(); // Resetta redo ad ogni nuova modifica
    }
}

void EditorState::undo() {
    if (undoStack.empty()) return;

    auto lastState = undoStack.back();
    undoStack.pop_back();

    redoStack.push_back(tilemap->getFullMapState());
    tilemap->setFullMapState(lastState);
}

void EditorState::redo() {
    if (redoStack.empty()) return;

    auto nextState = redoStack.back();
    redoStack.pop_back();

    undoStack.push_back(tilemap->getFullMapState());
    tilemap->setFullMapState(nextState);
}

void EditorState::updateHoverSelection() {
    if (ImGui::GetIO().WantCaptureMouse) {
        return;
    }

    if (selectedEntity.type != SelectedEntity::Type::None) {
        // Se ho già selezionato qualcosa ➔ aggiorno solo il pannello con la selezione
        propertyPanel.setSelectedEntity(selectedEntity);
        return;
    }


    sf::Vector2i pixelPos = sf::Mouse::getPosition(game.getWindow());
    sf::Vector2f worldPos = game.getWindow().mapPixelToCoords(pixelPos, game.getGameView());
    int x = static_cast<int>(worldPos.x / tileSize);
    int y = static_cast<int>(worldPos.y / tileSize);

    if (x < 0 || x >= mapWidth || y < 0 || y >= mapHeight) {
        hoveredEntity = {}; // Reset
        propertyPanel.setSelectedEntity({});
        return;
    }

    // Prima controlliamo gruppi
    for (const auto& placed : placedGroups) {
        if (placed.position == sf::Vector2i(x, y)) {
            hoveredEntity.type = SelectedEntity::Type::Group;
            hoveredEntity.groupName = placed.groupName;
            hoveredEntity.groupPosition = placed.position;
            hoveredEntity.groupLayer = placed.layer;
            propertyPanel.setSelectedEntity(hoveredEntity);
            return;
        }
    }

    // Poi tile
    TileInstance tile = tilemap->getTile(currentLayerIndex, x, y);
    if (tile.isValid()) {
        hoveredEntity.type = SelectedEntity::Type::Tile;
        hoveredEntity.tile = tile;
        hoveredEntity.tilePosition = sf::Vector2i(x, y); // <<< ti manca questo!
        propertyPanel.setSelectedEntity(hoveredEntity);
        return;
    }

    // Nessun risultato
    hoveredEntity = {};
    propertyPanel.setSelectedEntity({});
}


sf::RectangleShape EditorState::getHighlightRectangle(const SelectedEntity& entity) {
    sf::RectangleShape rect;

    if (entity.type == SelectedEntity::Type::Tile) {
        rect.setSize(sf::Vector2f(tileSize, tileSize));
        rect.setPosition(entity.tilePosition.x * tileSize, entity.tilePosition.y * tileSize);
    }
    else if (entity.type == SelectedEntity::Type::Group) {
        for (const auto& group : groupEditor->getGroups()) {
            if (group.name == entity.groupName) {
                int w = group.cols * tileSize;
                int h = group.rows * tileSize;
                rect.setSize(sf::Vector2f(w, h));
                rect.setPosition(entity.groupPosition.x * tileSize, entity.groupPosition.y * tileSize);
                break;
            }
        }
    }
    else if (entity.type == SelectedEntity::Type::Entity && entity.entity) {
        rect.setSize(sf::Vector2f(tileSize, tileSize));
        rect.setPosition(entity.entity->getX() * tileSize, entity.entity->getY() * tileSize);
    }

    return rect;
}


