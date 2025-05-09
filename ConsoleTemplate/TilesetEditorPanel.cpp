// TilesetEditorPanel.cpp
#include "TilesetEditorPanel.h"
#include "TileSetManager.h"
#include "imgui.h"
#include "ImGuiFileDialog.h"
#include "AnimationGroup.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include <iostream>
using json = nlohmann::json;

struct AnimationGroupPart {
    std::string partName;
    std::vector<int> frameIDs;
    int posX = 0;
    int posY = 0;

    AnimationGroupPart() = default;
};

struct AnimationGridCell {
    std::vector<int> frameIDs;
};


/*struct AnimationGroup {
    std::string name;
    float frameDuration = 0.2f;
    int gridCols = 1;
    int gridRows = 1;
    std::vector<AnimationGroupPart> parts;
    std::vector<std::vector<AnimationGroupPart>> grid;
};*/

int tileConfigID = 0;
std::unordered_map<int, TileInfo> tileMetadata;
static std::vector<AnimationGroup> animationGroups;
static char animationGroupName[64] = "";
static float animationGroupDuration = 0.2f;
static char animationPartName[64] = "";
static std::vector<int> currentPartFrameIDs;
static int dragTargetX = -1, dragTargetY = -1;
static int animationGridCols = 1;
static int animationGridRows = 1;
static int animationGroupCols = 1;
static int animationGroupRows = 1;
static int selectedCellX = -1;
static int selectedCellY = -1;
static int selectedGridX = -1, selectedGridY = -1;

namespace fs = std::filesystem;

bool isTileFullyTransparent(const sf::Image& img, int x, int y, int tileW, int tileH) {
    for (int i = 0; i < tileW; ++i) {
        for (int j = 0; j < tileH; ++j) {
            if (img.getPixel(x + i, y + j).a > 0) {
                return false;
            }
        }
    }
    return true;
}



TilesetEditorPanel::TilesetEditorPanel(TileSetManager& manager)
    : tilesetManager(manager) {
    loadMetadataFromJson("assets/metadata.json");                     // tile
    loadAnimationGroupsFromJson("assets/animation_groups.json");     // gruppi animati
    loadMetadataFromJson("assets/metadata.json");
}


void TilesetEditorPanel::setTileSize(int width, int height) {
    tileWidth = width;
    tileHeight = height;
}

void TilesetEditorPanel::render(bool* open) {
    if (!ImGui::Begin("Gestione Tileset", open)) {
        ImGui::End();
        return;
    }

    renderTileGridSection();
    ImGui::Separator();
    renderTileProperties();
    ImGui::Separator();
    //renderAnimationGroupEditor();
    //ImGui::Separator();
    //renderAnimationPreviewSection();
    //ImGui::Separator();
    //renderAnimationGroupPlacement();

    ImGui::End();
}

void TilesetEditorPanel::renderCompositeAnimationPreview() {
    ImGui::Begin("Preview Gruppo Animato");

    static float timer = 0.0f;
    timer += ImGui::GetIO().DeltaTime;

    const sf::Texture* texture = tilesetManager.getTexture(selectedTilesetName);
    if (!texture) {
        ImGui::Text("Nessun tileset selezionato.");
        ImGui::End();
        return;
    }

    int columns = texture->getSize().x / tileWidth;

    for (const auto& group : animationGroups) {
        ImGui::Text("Gruppo: %s (%.2fs)", group.name.c_str(), group.frameDuration);

        if (group.frames.empty()) continue;

        int frameCount = static_cast<int>(group.frames.size());
        int currentFrame = static_cast<int>((timer / group.frameDuration)) % frameCount;

        const auto& frameTiles = group.frames[currentFrame];

        ImGui::BeginGroup();

        for (const auto& tile : frameTiles) {
            int tileID = tile.tileID;
            sf::Vector2i offset = tile.offset;

            int tx = (tileID % columns) * tileWidth;
            int ty = (tileID / columns) * tileHeight;

            ImVec2 uv0((float)(tx) / texture->getSize().x, (float)(ty) / texture->getSize().y);
            ImVec2 uv1((float)(tx + tileWidth) / texture->getSize().x, (float)(ty + tileHeight) / texture->getSize().y);

            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offset.x * 64.0f);
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + offset.y * 64.0f);

            ImGui::Image((void*)(intptr_t)texture->getNativeHandle(), ImVec2(64, 64), uv0, uv1);
        }

        ImGui::EndGroup();
    }


    ImGui::End();
}


/*void TilesetEditorPanel::renderAnimationPreviewSection() {
    ImGui::Begin("Anteprima Gruppo Animazione");

    static float timer = 0.0f;
    timer += ImGui::GetIO().DeltaTime;

    static char pathBuffer[128] = "assets/animation_groups.json";
    static int selectedGroupIndex = 0;

    ImGui::InputText("Percorso JSON", pathBuffer, IM_ARRAYSIZE(pathBuffer));
    if (ImGui::Button("Carica Gruppo Animazioni")) {
        loadAnimationGroupsFromJson("assets/animation_groups.json");

    }

    if (ImGui::BeginPopup("Errore")) {
        ImGui::Text("Errore apertura JSON");
        if (ImGui::Button("OK")) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    if (!animationGroups.empty()) {
        std::vector<const char*> groupNames;
        for (const auto& group : animationGroups) {
            groupNames.push_back(group.name.c_str());
        }

        ImGui::Combo("Seleziona Gruppo", &selectedGroupIndex, groupNames.data(), (int)groupNames.size());
        AnimationGroup& group = animationGroups[selectedGroupIndex];

        ImGui::Text("%s (%.2fs)", group.name.c_str(), group.frameDuration);

        const sf::Texture* texture = tilesetManager.getTexture(selectedTilesetName);
        if (texture && !group.frames.empty()) {
            int columns = texture->getSize().x / tileWidth;
            int frameCount = static_cast<int>(group.frames.size());
            int currentFrame = static_cast<int>((timer / group.frameDuration)) % frameCount;

            const auto& frame = group.frames[currentFrame];

            ImGui::BeginChild("AnimPreview", ImVec2(0, 300), true);
            for (const auto& tile : frame) {
                int tileID = tile.tileID;
                sf::Vector2i offset = tile.offset;

                int tx = (tileID % columns) * tileWidth;
                int ty = (tileID / columns) * tileHeight;

                ImVec2 uv0((float)(tx) / texture->getSize().x, (float)(ty) / texture->getSize().y);
                ImVec2 uv1((float)(tx + tileWidth) / texture->getSize().x, (float)(ty + tileHeight) / texture->getSize().y);

                ImGui::SetCursorPos(ImVec2(offset.x * 48.0f, offset.y * 48.0f));
                ImGui::Image((void*)(intptr_t)texture->getNativeHandle(), ImVec2(48, 48), uv0, uv1);
            }
            ImGui::EndChild();

            // Controlli per inserire un nuovo tile
            static int cellX = 0;
            static int cellY = 0;
            ImGui::InputInt("Posizione X", &cellX);
            ImGui::InputInt("Posizione Y", &cellY);

            static int frameToEdit = 0;
            ImGui::InputInt("Frame # da modificare", &frameToEdit);

            if (ImGui::Button("Aggiungi Tile Selezionato")) {
                if (frameToEdit >= 0 && frameToEdit < group.frames.size()) {
                    group.frames[frameToEdit].push_back({ tileConfigID, {cellX, cellY} });
                    std::cout << "[Editor] Aggiunto tile ID " << tileConfigID << " al frame " << frameToEdit
                        << " in (" << cellX << ", " << cellY << ")\n";
                }
            }
        }
    }


    ImGui::End();
}*/


void TilesetEditorPanel::renderTileGridSection() {
    if (ImGui::Button("Carica tileset...")) {
        ImGuiFileDialog::Instance()->OpenDialog("CaricaTileset", "Seleziona tileset PNG", ".png", IGFD::FileDialogConfig{ "." });
    }

    if (ImGuiFileDialog::Instance()->Display("CaricaTileset")) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            std::string filePath = ImGuiFileDialog::Instance()->GetFilePathName();
            loadTilesetFromFile(filePath);
        }
        ImGuiFileDialog::Instance()->Close();
    }

    ImGui::Separator();
    ImGui::Text("Tileset Caricati:");

    for (int i = 0; i < loadedTilesets.size(); ++i) {
        TilesetEntry& entry = loadedTilesets[i];

        ImGui::PushID(i);
        ImGui::Text("%s", entry.name.c_str());

        const char* stagioni[] = { "tutte", "estate", "inverno", "autunno", "primavera" };
        int selected = 0;
        for (int s = 0; s < 5; ++s) {
            if (entry.season == stagioni[s]) selected = s;
        }
        if (ImGui::Combo("Stagione", &selected, stagioni, IM_ARRAYSIZE(stagioni))) {
            entry.season = stagioni[selected];
        }

        ImGui::SameLine();
        if (ImGui::Button("X")) {
            loadedTilesets.erase(loadedTilesets.begin() + i);
            ImGui::PopID();
            break;
        }
        ImGui::Separator();
        ImGui::PopID();
    }
    


    ImGui::Separator();
    ImGui::Text("Tileset:");
    drawTileGrid(); // <--- griglia di selezione dei tile
}


// Versione migliorata del pannello TileInfo con campi precompilati e dropdown
void TilesetEditorPanel::renderTileProperties() {
    ImGui::Separator();
    ImGui::Text("Proprieta' tile:");

    ImGui::InputInt("##TileID", &tileConfigID);
    ImGui::SameLine();
    ImGui::Text("ID");

    if (tileMetadata.find(tileConfigID) == tileMetadata.end()) {
        TileInfo autoInfo;
        autoInfo.animated = false;
       

        if (selectedTilesetName.find("Grass") != std::string::npos) {
            autoInfo.category = "vegetazione";
            autoInfo.genre = "naturale";
            autoInfo.type = "decor";
        }
        else if (selectedTilesetName.find("Rock") != std::string::npos) {
            autoInfo.category = "roccia";
            autoInfo.genre = "naturale";
            autoInfo.type = "blocco";
        }

        if (selectedTilesetName.find("Spring") != std::string::npos)
            autoInfo.season = "spring";
        else if (selectedTilesetName.find("Summer") != std::string::npos)
            autoInfo.season = "summer";
        else if (selectedTilesetName.find("Fall") != std::string::npos)
            autoInfo.season = "fall";
        else if (selectedTilesetName.find("Winter") != std::string::npos)
            autoInfo.season = "winter";

        tileMetadata[tileConfigID] = autoInfo;
        //autoFillDone = true;
    }

    TileInfo& info = tileMetadata[tileConfigID];

    static char nameBuffer[64] = "";
    strncpy_s(nameBuffer, sizeof(nameBuffer), info.name.c_str(), _TRUNCATE);
    ImGui::InputText("Nome", nameBuffer, IM_ARRAYSIZE(nameBuffer));
    info.name = nameBuffer;

    const char* types[] = { "decor", "blocco", "porta", "trigger" };
    static int selectedType = 0;
    for (int i = 0; i < 4; ++i) {
        if (info.type == types[i]) selectedType = i;
    }
    if (ImGui::Combo("Tipo", &selectedType, types, IM_ARRAYSIZE(types))) {
        info.type = types[selectedType];
    }

    ImGui::Checkbox("Camminabile", &info.walkable);

    const char* categories[] = { "vegetazione", "acqua", "architettura", "roccia" };
    static int selectedCategory = 0;
    for (int i = 0; i < 4; ++i) {
        if (info.category == categories[i]) selectedCategory = i;
    }
    if (ImGui::Combo("Categoria", &selectedCategory, categories, IM_ARRAYSIZE(categories))) {
        info.category = categories[selectedCategory];
    }

    const char* genres[] = { "naturale", "magico", "urbano", "oscuro" };
    static int selectedGenre = 0;
    for (int i = 0; i < 4; ++i) {
        if (info.genre == genres[i]) selectedGenre = i;
    }
    if (ImGui::Combo("Genere", &selectedGenre, genres, IM_ARRAYSIZE(genres))) {
        info.genre = genres[selectedGenre];
    }

    const char* seasons[] = { "all", "spring", "summer", "fall", "winter" };
    int selectedSeason = std::find(seasons, seasons + 5, info.season) - seasons;
    if (ImGui::Combo("Stagione", &selectedSeason, seasons, IM_ARRAYSIZE(seasons))) {
        info.season = seasons[selectedSeason];
    }

    ImGui::Checkbox("Animato", &info.animated);
    if (info.animated) {
        ImGui::InputInt("Frame Count", &info.frameCount);
        ImGui::InputFloat("Frame Duration", &info.frameDuration);
    }

    if (ImGui::Button("Salva Tile")) {
        tileMetadata[tileConfigID] = info;
        std::cout << "[Editor] Salvato tile ID " << tileConfigID << "\n";
    }

    if (ImGui::Button("Esporta metadata.json")) {
        saveMetadataToJson("assets/metadata.json");
    }
    if (ImGui::Button("Genera Lua script")) {
        generateLuaScript("assets/world_script.lua");
    }
}


void TilesetEditorPanel::renderAnimationEditorSection() {
    static std::vector<int> selectedFrames;
    static char animName[64] = "";
    static float frameDuration = 0.15f;

    ImGui::Text("Editor Animazioni:");

    if (ImGui::Button("Inizia selezione animazione")) {
        selectedFrames.clear();
    }

    ImGui::SameLine();
    if (ImGui::Button("Aggiungi frame corrente")) {
        selectedFrames.push_back(tileConfigID);
    }

    ImGui::Text("Frame selezionati:");
    for (int id : selectedFrames) {
        ImGui::BulletText("ID %d", id);
    }

    ImGui::InputFloat("Durata frame", &frameDuration);
    ImGui::InputText("Nome animazione", animName, IM_ARRAYSIZE(animName));

    if (ImGui::Button("Salva animazione")) {
        std::ofstream out("assets/animations/" + std::string(animName) + ".json");
        if (out.is_open()) {
            out << "{\n  \"name\": \"" << animName << "\",\n";
            out << "  \"tileIDs\": [";
            for (size_t i = 0; i < selectedFrames.size(); ++i) {
                out << selectedFrames[i];
                if (i + 1 < selectedFrames.size()) out << ", ";
            }
            out << "],\n  \"frameDuration\": " << frameDuration << "\n}";
            std::cout << "[Animazione] Salvata animazione " << animName << "\n";
        }
    }
}


/*void TilesetEditorPanel::renderAnimationGroupEditor() {
    static char groupName[64] = "";
    static float duration = 0.2f;
    static int cols = 1, rows = 1;

    ImGui::InputText("Nome Gruppo", groupName, IM_ARRAYSIZE(groupName));
    ImGui::InputFloat("Durata Frame", &duration);
    ImGui::InputInt("Colonne", &cols);
    ImGui::InputInt("Righe", &rows);

    if (ImGui::Button("Crea o Seleziona Gruppo")) {
        auto it = std::find_if(animationGroups.begin(), animationGroups.end(),
            [&](const AnimationGroup& g) { return g.name == groupName; });

        if (it == animationGroups.end()) {
            AnimationGroup group;
            group.name = groupName;
            group.frameDuration = duration;
            group.cols = cols;
            group.rows = rows;
            group.frames.resize(1); // inizia con 1 frame vuoto
            animationGroups.push_back(group);
        }
    }

    AnimationGroup* selectedGroup = nullptr;
    for (auto& g : animationGroups) {
        if (g.name == groupName) selectedGroup = &g;
    }
    if (!selectedGroup) return;

    static int selectedFrame = 0;
    ImGui::InputInt("Frame da modificare", &selectedFrame);
    if (selectedFrame < 0) selectedFrame = 0;
    if (selectedFrame >= selectedGroup->frames.size()) selectedGroup->frames.resize(selectedFrame + 1);

    ImGui::InputInt("X", &selectedGridX);
    ImGui::InputInt("Y", &selectedGridY);

    if (ImGui::Button("Aggiungi Tile Selezionato alla cella")) {
        selectedGroup->frames[selectedFrame].push_back({ tileConfigID, { selectedGridX, selectedGridY } });
        std::cout << "[Editor] Aggiunto tile ID " << tileConfigID
            << " al frame " << selectedFrame
            << " in posizione (" << selectedGridX << ", " << selectedGridY << ")\n";
    }

    if (ImGui::Button("Salva Gruppo Animazione")) {
        saveAnimationGroupsToJson("assets/animation_groups.json");
    }
}*/


/*void TilesetEditorPanel::renderInteractiveAnimationGroupEditor() {
    ImGui::InputText("Nome Gruppo", animationGroupName, IM_ARRAYSIZE(animationGroupName));
    ImGui::InputFloat("Durata Frame", &animationGroupDuration);
    ImGui::InputInt("Colonne", &animationGridCols);
    ImGui::InputInt("Righe", &animationGridRows);

    static int currentFrame = 0;
    ImGui::InputInt("Frame da modificare", &currentFrame);

    ImGui::Text("Griglia: clicca su una cella");
    for (int y = 0; y < animationGridRows; ++y) {
        for (int x = 0; x < animationGridCols; ++x) {
            std::string label = "##cell_" + std::to_string(x) + "_" + std::to_string(y);
            if (ImGui::Button(label.c_str(), ImVec2(32, 32))) {
                selectedCellX = x;
                selectedCellY = y;
            }
            if (x < animationGridCols - 1) ImGui::SameLine();
        }
    }

    if (ImGui::Button("Aggiungi Tile Selezionato")) {
        auto it = std::find_if(animationGroups.begin(), animationGroups.end(),
            [&](const AnimationGroup& g) { return g.name == animationGroupName; });

        if (it == animationGroups.end()) {
            AnimationGroup newGroup;
            newGroup.name = animationGroupName;
            newGroup.frameDuration = animationGroupDuration;
            newGroup.cols = animationGridCols;
            newGroup.rows = animationGridRows;
            newGroup.frames.resize(currentFrame + 1);
            animationGroups.push_back(newGroup);
            it = std::prev(animationGroups.end());
        }

        if (currentFrame >= it->frames.size())
            it->frames.resize(currentFrame + 1);

        it->frames[currentFrame].push_back({ tileConfigID, { selectedCellX, selectedCellY } });

        std::cout << "[Editor] Aggiunto tile ID " << tileConfigID
            << " a frame " << currentFrame
            << " in posizione (" << selectedCellX << ", " << selectedCellY << ")\n";
    }

    if (ImGui::Button("Salva Gruppo Animazione")) {
        std::ofstream out("assets/animation_groups.json");
        json j;
        for (const auto& group : animationGroups) {
            json groupJson;
            groupJson["frameDuration"] = group.frameDuration;
            groupJson["cols"] = group.cols;
            groupJson["rows"] = group.rows;

            for (const auto& frame : group.frames) {
                json frameArray;
                for (const auto& tile : frame) {
                    frameArray.push_back({
                        {"tileID", tile.tileID},
                        {"offset", { tile.offset.x, tile.offset.y }}
                        });
                }
                groupJson["frames"].push_back(frameArray);
            }

            j[group.name] = groupJson;
        }
        out << std::setw(4) << j;
        out.close();
    }

    ImGui::Text("Gruppi salvati:");
    for (const auto& group : animationGroups) {
        ImGui::Text("- %s (%dx%d, %d frame)", group.name.c_str(), group.cols, group.rows, (int)group.frames.size());
    }
}*/


void TilesetEditorPanel::renderAnimationGroupPlacement() {
    static int selectedGroupIndex = 0;
    if (animationGroups.empty()) return;

    ImGui::Begin("Posiziona Gruppo Animato");
    std::vector<const char*> names;
    for (const auto& g : animationGroups) names.push_back(g.name.c_str());
    ImGui::Combo("Gruppo", &selectedGroupIndex, names.data(), names.size());

    if (ImGui::Button("Posiziona sulla Mappa")) {
        const auto& group = animationGroups[selectedGroupIndex];
        // TODO: posizionamento reale in mappa
        std::cout << "[Editor] Posizionamento gruppo animato: " << group.name << "\n";
    }
    ImGui::End();
}







void TilesetEditorPanel::drawTileGrid() {
    if (loadedTilesets.empty()) return;

    static std::string currentSeason = loadedTilesets[selectedTilesetIndex].season;

    if (ImGui::IsWindowFocused() && ImGui::IsKeyPressed(ImGuiKey_Tab)) {
        for (int i = selectedTilesetIndex + 1; i < loadedTilesets.size(); ++i) {
            if (loadedTilesets[i].season == currentSeason) {
                selectedTilesetIndex = i;
                selectedTilesetName = loadedTilesets[i].name;
                break;
            }
        }
    }

    ImGui::BeginChild("TileGrid", ImVec2(0, 400), true,
        ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_AlwaysVerticalScrollbar);

    for (int t = 0; t < loadedTilesets.size(); ++t) {
        const sf::Texture* texture = tilesetManager.getTexture(loadedTilesets[t].name);
        if (!texture) continue;

        sf::Image image = texture->copyToImage();
        int columns = texture->getSize().x / tileWidth;
        int rows = texture->getSize().y / tileHeight;
        int maxTileID = columns * rows;

        ImTextureID texID = (ImTextureID)(uintptr_t)texture->getNativeHandle();
        float displaySize = 48.0f;
        float spacing = 2.0f;
        float scrollX = ImGui::GetScrollX();
        ImVec2 origin = ImGui::GetCursorScreenPos();

        ImVec2 contentSize = ImVec2(columns * (displaySize + spacing), rows * (displaySize + spacing));
        ImGui::Dummy(contentSize);
        ImDrawList* drawList = ImGui::GetWindowDrawList();

        for (int i = 0; i < maxTileID; ++i) {
            int col = i % columns;
            int row = i / columns;
            int tx = col * tileWidth;
            int ty = row * tileHeight;

            if (isTileFullyTransparent(image, tx, ty, tileWidth, tileHeight)) continue;

            ImVec2 pos = ImVec2(origin.x + col * (displaySize + spacing) - scrollX,
                origin.y + row * (displaySize + spacing));
            ImVec2 size = ImVec2(displaySize, displaySize);

            ImVec2 uv0((float)(tx) / texture->getSize().x, (float)(ty) / texture->getSize().y);
            ImVec2 uv1((float)(tx + tileWidth) / texture->getSize().x, (float)(ty + tileHeight) / texture->getSize().y);

            ImGui::PushID(t * 10000 + i);
            if (ImGui::IsMouseHoveringRect(pos, ImVec2(pos.x + size.x, pos.y + size.y)) && ImGui::IsMouseClicked(0)) {
                selectedTileID = i;
                tileConfigID = i;

                selectedTilesetIndex = t;
                selectedTilesetName = loadedTilesets[t].name;

                if (onTilesetSelected) onTilesetSelected(selectedTilesetName);
                if (onTileSelected) onTileSelected(selectedTileID);

                if (tileMap) tileMap->setCurrentTileset(selectedTilesetName);
            }

            drawList->AddImage(texID, pos, ImVec2(pos.x + size.x, pos.y + size.y), uv0, uv1);

            // Disegna simbolo animazione
            auto it = tileMetadata.find(i);
            if (it != tileMetadata.end() && it->second.animated) {
                ImVec2 iconPos = ImVec2(pos.x + 2, pos.y + 2);
                drawList->AddText(ImGui::GetFont(), 10.0f, iconPos, IM_COL32(255, 255, 0, 255), "★");
            }

            if (i == selectedTileID) {
                drawList->AddRect(pos, ImVec2(pos.x + size.x, pos.y + size.y), IM_COL32(255, 0, 0, 255), 0.f, 0, 2.f);
            }

            ImGui::PopID();
        }
    }

    ImGui::EndChild();
}


void TilesetEditorPanel::saveMetadataToJson(const std::string& path) {
    json j;
    for (const auto& [id, info] : tileMetadata) {
        j[std::to_string(id)] = {
            {"name", info.name},
            {"type", info.type},
            {"walkable", info.walkable},
            {"category", info.category},
            {"genre", info.genre},
            {"season", info.season},
            {"animated", info.animated},
            {"frameCount", info.frameCount},
            {"frameDuration", info.frameDuration}
        };
    }

    std::ofstream out(path);
    out << std::setw(4) << j;
}


void TilesetEditorPanel::loadMetadataFromJson(const std::string& path) {
    std::ifstream in(path);
    if (!in.is_open()) {
        std::cerr << "[Editor] Impossibile aprire il file: " << path << "\n";
        return;
    }

    json j;
    in >> j;

    for (auto& [idStr, value] : j.items()) {
        int id = std::stoi(idStr);
        TileInfo info;
        info.name = value.value("name", "Sconosciuto");
        info.type = value.value("type", "Unknown");
        info.walkable = value.value("walkable", true);
        info.category = value.value("category", "Generico");
        info.genre = value.value("genre", "Nessuno");
        info.season = value.value("season", "all");
        info.animated = value.value("animated", false);
        info.frameCount = value.value("frameCount", 1);
        info.frameDuration = value.value("frameDuration", 0.1f);

        tileMetadata[id] = info;
    }

    std::cout << "[Editor] Caricati " << tileMetadata.size() << " metadati tile da " << path << "\n";
}

void TilesetEditorPanel::saveAnimationGroupsToJson(const std::string& path) {
    json j;
    for (const auto& group : animationGroups) {
        json groupJson;
        groupJson["frameDuration"] = group.frameDuration;
        groupJson["cols"] = group.cols;
        groupJson["rows"] = group.rows;

        for (const auto& frame : group.frames) {
            json frameArray;
            for (const auto& tile : frame) {
                frameArray.push_back({
                    {"tileID", tile.tileID},
                    {"offset", { tile.offset.x, tile.offset.y }}
                    });
            }
            groupJson["frames"].push_back(frameArray);
        }

        j[group.name] = groupJson;
    }

    std::ofstream out(path);
    if (!out) {
        std::cerr << "[Save] Errore apertura file: " << path << "\n";
        return;
    }

    out << std::setw(4) << j;
    std::cout << "[Save] Salvati " << animationGroups.size() << " gruppi animati in " << path << "\n";
}

void TilesetEditorPanel::loadAnimationGroupsFromJson(const std::string& path) {
    std::ifstream in(path);
    if (!in.is_open()) {
        std::cerr << "[Load] Errore apertura file: " << path << "\n";
        return;
    }

    json j;
    in >> j;

    animationGroups.clear();

    for (const auto& [groupName, groupData] : j.items()) {
        AnimationGroup group;
        group.name = groupName;
        group.frameDuration = groupData.value("frameDuration", 0.2f);
        group.cols = groupData.value("cols", 1);
        group.rows = groupData.value("rows", 1);

        const auto& framesArray = groupData["frames"];
        for (const auto& frameJson : framesArray) {
            std::vector<AnimationGroupFrame> frame;
            for (const auto& tileJson : frameJson) {
                AnimationGroupFrame f;
                f.tileID = tileJson["tileID"];
                const auto& offset = tileJson["offset"];
                f.offset = sf::Vector2i(offset[0], offset[1]);
                frame.push_back(f);
            }
            group.frames.push_back(frame);
        }

        animationGroups.push_back(group);
    }

    std::cout << "[Load] Caricati " << animationGroups.size() << " gruppi da " << path << "\n";
}



void TilesetEditorPanel::generateLuaScript(const std::string& path) {
    std::ofstream out(path);
    out << "-- Lua script generato automaticamente\n";
    for (const auto& [id, info] : tileProperties) {
        out << "tiles[" << id << "] = { name = '" << info.name << "', type = '" << info.type
            << "', walkable = " << (info.walkable ? "true" : "false")
            << ", category = '" << info.category << "', genre = '" << info.genre << "' }\n";
    }
}


void TilesetEditorPanel::loadTilesetFromFile(const std::string& filePath) {
    sf::Image image;
    if (!image.loadFromFile(filePath)) {
        std::cerr << "[TilesetEditor] Errore caricamento PNG: " << filePath << "\n";
        return;
    }

    image.createMaskFromColor(sf::Color(255, 0, 255));

    std::string name = fs::path(filePath).stem().string();
    tilesetManager.loadTileset(name, filePath);

    loadedTilesets.push_back({ name, filePath });
    selectedTilesetName = name;
    selectedTilesetIndex = static_cast<int>(loadedTilesets.size()) - 1;
}
