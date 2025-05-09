#include "AnimationGroupEditorPanel.h"
#include "imgui.h"
#include "ImGuiFileDialog.h"
#include <fstream>
#include <iostream>
#include <chrono>

// Tag predefiniti visivi
static const char* presetTags[] = { "decor", "fuoco", "acqua", "pericolo", "magia", "porta" };
char tagFilter[32] = ""; // buffer globale per filtro tag

static int selectedPlacedInstance = -1;
static bool placingInstanceMove = false;

static std::unordered_map<std::string, ImVec4> tagColors = {
    { "decor",     ImVec4(0.5f, 0.7f, 1.0f, 1.0f) },
    { "fuoco",     ImVec4(1.0f, 0.3f, 0.1f, 1.0f) },
    { "acqua",     ImVec4(0.1f, 0.5f, 1.0f, 1.0f) },
    { "pericolo",  ImVec4(1.0f, 0.1f, 0.1f, 1.0f) },
    { "magia",     ImVec4(0.6f, 0.2f, 0.8f, 1.0f) },
    { "porta",     ImVec4(0.8f, 0.8f, 0.4f, 1.0f) }
};


using json = nlohmann::json; // Alias per la libreria JSON nlohmann

// Timer globale per preview fluida
static std::chrono::steady_clock::time_point lastPreviewTime = std::chrono::steady_clock::now();
static float previewElapsed = 0.0f;


// Costruttore del pannello: inizializza il riferimento al TileSetManager
AnimationGroupEditorPanel::AnimationGroupEditorPanel(TileSetManager& manager)
    : tileSetManager(manager) {
    std::memset(groupNameBuffer, 0, sizeof(groupNameBuffer)); // Inizializza il buffer del nome del gruppo
}

// Funzione principale di rendering del pannello editor
// Funzione principale di rendering del pannello editor
void AnimationGroupEditorPanel::render(bool* open, bool& placingGroup, std::string& currentGroupToPlace) {
    // Backup automatico ogni 30 secondi se ci sono modifiche
    static float autoSaveTimer = 0.0f;
    static std::vector<AnimationGroup> lastSavedGroups = groups; // copia iniziale per confronto
    autoSaveTimer += ImGui::GetIO().DeltaTime;



    if (autoSaveTimer > 30.0f) { // ogni 30 secondi
        if (groups != lastSavedGroups) {
            std::ofstream backup("assets/animation_groups_backup.json");
            if (backup.is_open()) {
                json j;
                for (const auto& group : groups) {
                    json g;
                    g["name"] = group.name;
                    g["duration"] = group.frameDuration;
                    g["cols"] = group.cols;
                    g["rows"] = group.rows;
                    g["frames"] = json::array();
                    for (const auto& frame : group.frames) {
                        json f;
                        for (const auto& tile : frame) {
                            f.push_back({
                                {"tileID", tile.tileID},
                                {"offset", {tile.offset.x, tile.offset.y}}
                                });
                        }
                        g["frames"].push_back(f);
                    }
                    j.push_back(g);
                }
                backup << j.dump(2);
                backup.close();
                lastSavedGroups = groups; // aggiorna backup
            }
        }
        autoSaveTimer = 0.0f;
    }
    if (!ImGui::Begin("Editor Gruppi Animati", open)) { // Inizia una finestra ImGui
        ImGui::End(); // Se non visibile, termina la finestra
        return;
    }

    // Calcola deltaTime per preview
    auto now = std::chrono::steady_clock::now();
    std::chrono::duration<float> delta = now - lastPreviewTime;
    previewElapsed += delta.count();
    lastPreviewTime = now;

    // Verifica che un tileset valido sia selezionato
    if (selectedTilesetName.empty() || !tileSetManager.getTexture(selectedTilesetName)) {
        ImGui::Text("Nessun tileset valido selezionato!"); // Messaggio di errore
        ImGui::End();
        return;
    }

    if (ImGui::Button("↩️ Undo") && !undoStack.empty()) {
        redoStack.push_back(groups);
        groups = undoStack.back();
        undoStack.pop_back();
    }

    ImGui::SameLine();
    if (ImGui::Button("↪️ Redo") && !redoStack.empty()) {
        undoStack.push_back(groups);
        groups = redoStack.back();
        redoStack.pop_back();
    }


    ImGui::Columns(3, nullptr, true); // Divide la UI in 3 colonne
    renderSidebar(placingGroup, currentGroupToPlace); // Colonna 1: selezione gruppi
    ImGui::NextColumn();
    renderGridComposer();                             // Colonna 2: composizione frame
    ImGui::NextColumn();
    renderGroupProperties();                          // Colonna 3: proprietà del gruppo selezionato

    ImGui::Columns(1);
    ImGui::Separator();
    renderPreview();                                  // Anteprima dell'animazione in fondo

    renderPlacedInstancePanel();
    // Disegna tutte le istanze piazzate nella mappa
    renderPlacedInstances();


    ImGui::End(); // Termina la finestra ImGui
}

// Sidebar laterale per visualizzare, selezionare e creare nuovi gruppi animati
void AnimationGroupEditorPanel::renderSidebar(bool& placingGroup, std::string& currentGroupToPlace)
{
    ImGui::Text("Gruppi:");

    // Cicla tutti i gruppi visibili
    for (int i = 0; i < groups.size(); ++i) {
        if (strlen(tagFilter) > 0 && groups[i].tag.find(tagFilter) == std::string::npos)
            continue;

        ImGui::PushID(i);

        // Checkbox per selezione multipla
        bool check = selectedGroups.count(i) > 0;
        if (ImGui::Checkbox("##chk", &check)) {
            if (check) selectedGroups.insert(i);
            else selectedGroups.erase(i);
        }
        ImGui::SameLine();

        // Selezione singola cliccabile per editing
        bool isSelected = (i == selectedGroupIndex);
        if (ImGui::Selectable(groups[i].name.c_str(), isSelected)) {
            selectedGroupIndex = i;
            strncpy_s(groupNameBuffer, groups[i].name.c_str(), sizeof(groupNameBuffer));
            frameDuration = groups[i].frameDuration;
            cols = groups[i].cols;
            rows = groups[i].rows;
        }

        ImGui::PopID();
    }


    // Pulsante "Piazza nella mappa" se selezionato
    if (selectedGroupIndex >= 0 && selectedGroupIndex < groups.size()) {
        if (ImGui::Button("Piazza nella Mappa")) {
            placingGroup = true;
            currentGroupToPlace = groups[selectedGroupIndex].name;
        }
    }

    


    static bool showError = false;
    static std::string errorMsg = "";

    // ➕ Nuovo gruppo
    if (ImGui::Button("+ Nuovo Gruppo")) {
        pushUndo();
        AnimationGroup newGroup;
        newGroup.name = "NuovoGruppo";
        newGroup.frameDuration = 0.2f;
        newGroup.cols = 3;
        newGroup.rows = 3;

        groups.push_back(newGroup);
        selectedGroupIndex = groups.size() - 1;
        strncpy_s(groupNameBuffer, newGroup.name.c_str(), sizeof(groupNameBuffer));
        showError = false;
    }

    // Preset tag e colori (già definiti sopra)
    static const char* presetTags[] = { "decor", "fuoco", "acqua", "pericolo", "magia", "porta" };
    static std::unordered_map<std::string, ImVec4> tagColors = {
        { "decor",     ImVec4(0.5f, 0.7f, 1.0f, 1.0f) },
        { "fuoco",     ImVec4(1.0f, 0.3f, 0.1f, 1.0f) },
        { "acqua",     ImVec4(0.1f, 0.5f, 1.0f, 1.0f) },
        { "pericolo",  ImVec4(1.0f, 0.1f, 0.1f, 1.0f) },
        { "magia",     ImVec4(0.6f, 0.2f, 0.8f, 1.0f) },
        { "porta",     ImVec4(0.8f, 0.8f, 0.4f, 1.0f) }
    };

    // CONFERMA MODIFICHE GRUPPO
    if (ImGui::Button("Conferma Modifiche Gruppo")) {

        // Se sei in modalità modifica multipla, salta la validazione nome
        if (multiEditMode) {
            showError = false;
            return;
        }

        std::string name = groupNameBuffer;

        // Controlla se esiste già un gruppo con lo stesso nome
        bool nameExists = false;
        for (int i = 0; i < groups.size(); ++i) {
            if (strlen(tagFilter) > 0 && groups[i].tag.find(tagFilter) == std::string::npos)
                continue;
            ImGui::PushID(i);

            // Checkbox per selezione multipla
            bool check = selectedGroups.count(i) > 0;
            if (ImGui::Checkbox("##chk", &check)) {
                if (check) selectedGroups.insert(i);
                else selectedGroups.erase(i);
            }

            ImGui::PopID();
        }
        

        // Validazioni solo per modifica singola
        if (name.empty()) {
            showError = true;
            errorMsg = "Il nome del gruppo non può essere vuoto.";
        }
        else if (name == "NuovoGruppo") {
            showError = true;
            errorMsg = "Rinomina il gruppo prima di confermare.";
        }
        else if (nameExists) {
            showError = true;
            errorMsg = "Esiste già un gruppo con questo nome.";
        }
        else if (frameDuration <= 0.0f) {
            showError = true;
            errorMsg = "La durata del frame deve essere positiva.";
        }
        else {
            groups[selectedGroupIndex].name = name;
            groups[selectedGroupIndex].frameDuration = frameDuration;
            groups[selectedGroupIndex].cols = cols;
            groups[selectedGroupIndex].rows = rows;
            showError = false;
        }
    }

    if (showError) {
        ImGui::TextColored(ImVec4(1, 0.4f, 0.4f, 1), "%s", errorMsg.c_str());
    }
}



void AnimationGroupEditorPanel::renderGridComposer() {
    if (selectedGroupIndex < 0 || selectedGroupIndex >= groups.size()) return;

    // QUI DEVI AGGIUNGERE :
    if (selectedTilesetName.empty() || !tileSetManager.getTexture(selectedTilesetName)) {
        ImGui::Text("Seleziona un tileset valido prima!");
        ImGui::End();
        return;
    }


    AnimationGroup& group = groups[selectedGroupIndex];
    if (selectedFrame >= group.frames.size()) group.frames.resize(selectedFrame + 1);

    ImGui::Text("Frame #%d", selectedFrame);
    ImGui::Separator();

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 origin = ImGui::GetCursorScreenPos();

    const sf::Texture* texture = tileSetManager.getTexture(selectedTilesetName);
    if (!texture) return;

    int columns = texture->getSize().x / tileWidth;
    float cellSize = 48.0f;

    for (int y = 0; y < group.rows; ++y) {
        for (int x = 0; x < group.cols; ++x) {
            ImVec2 cellPos = ImVec2(origin.x + x * cellSize, origin.y + y * cellSize);
            ImVec2 cellMax = ImVec2(cellPos.x + cellSize, cellPos.y + cellSize);
            drawList->AddRect(cellPos, cellMax, IM_COL32(255, 255, 255, 64));

            for (auto& tile : group.frames[selectedFrame]) {
                if (tile.offset.x == x && tile.offset.y == y) {
                    int tx = (tile.tileID % columns) * tileWidth;
                    int ty = (tile.tileID / columns) * tileHeight;
                    ImVec2 uv0(tx / (float)texture->getSize().x, ty / (float)texture->getSize().y);
                    ImVec2 uv1((tx + tileWidth) / (float)texture->getSize().x, (ty + tileHeight) / (float)texture->getSize().y);

                    drawList->AddImage(
                        (ImTextureID)(uintptr_t)texture->getNativeHandle(),
                        cellPos, cellMax, uv0, uv1
                    );
                }
            }
        }
    }

    ImGui::InvisibleButton("grid_area", ImVec2(group.cols * cellSize, group.rows * cellSize));
    if (ImGui::IsItemHovered()) {
        ImVec2 mouse = ImGui::GetMousePos();
        int x = static_cast<int>((mouse.x - origin.x) / cellSize);
        int y = static_cast<int>((mouse.y - origin.y) / cellSize);
        hoveredCellX = x;
        hoveredCellY = y;

        if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(0)) {
            ImVec2 mouse = ImGui::GetMousePos();
            int x = static_cast<int>((mouse.x - origin.x) / cellSize);
            int y = static_cast<int>((mouse.y - origin.y) / cellSize);
            handleMapClick(sf::Vector2i(x, y));
            trySelectPlacedInstanceAt(sf::Vector2i(x, y)); // 👈seleziona l'istanza cliccata

            hoveredCellX = x;
            hoveredCellY = y;

            // Questo è il punto chiave:
            handleMapClick(sf::Vector2i(x, y)); // Sposta istanza selezionata se stai cliccando per spostare

            if (currentTileID >= 0)
                groups[selectedGroupIndex].frames[selectedFrame].push_back({ currentTileID, {x, y} });
        }


    }

}

void AnimationGroupEditorPanel::renderGroupProperties() {
    if (selectedGroupIndex < 0 || selectedGroupIndex >= groups.size()) return;

    ImGui::Checkbox("Modifica Multipla", &multiEditMode);
    if (multiEditMode && selectedGroups.empty()) {
        ImGui::TextColored(ImVec4(1, 0.4f, 0.4f, 1), "Seleziona almeno un gruppo a sinistra.");
        return;
    }

    AnimationGroup& group = groups[selectedGroupIndex];

    // 📝 NOME GRUPPO
    if (!multiEditMode) {
        ImGui::InputText("Nome Gruppo", groupNameBuffer, IM_ARRAYSIZE(groupNameBuffer));
    }
    else {
        static char multiNameBuffer[64] = "";
        if (ImGui::InputText("Nome Multiplo", multiNameBuffer, sizeof(multiNameBuffer))) {
            if (strlen(multiNameBuffer) > 0) {
                for (int idx : selectedGroups) {
                    groups[idx].name = multiNameBuffer;
                }
            }
        }
    }

    // 🕒 DURATA FRAME
    ImGui::InputFloat("Durata Frame", &frameDuration);
    if (multiEditMode) {
        for (int idx : selectedGroups)
            groups[idx].frameDuration = frameDuration;
    }
    else {
        group.frameDuration = frameDuration;
    }

    // 🔢 FRAME #
    if (ImGui::InputInt("Frame #", &selectedFrame)) {
        if (selectedFrame < 0) selectedFrame = 0;
        if (selectedFrame >= group.frames.size()) {
            group.frames.resize(selectedFrame + 1);
        }
    }

    // 📐 COLONNE
    if (ImGui::InputInt("Colonne", &cols)) {
        if (multiEditMode) {
            for (int idx : selectedGroups)
                groups[idx].cols = std::max(1, cols);
        }
        else {
            group.cols = std::max(1, cols);
        }
    }

    // 📏 RIGHE
    if (ImGui::InputInt("Righe", &rows)) {
        if (multiEditMode) {
            for (int idx : selectedGroups)
                groups[idx].rows = std::max(1, rows);
        }
        else {
            group.rows = std::max(1, rows);
        }
    }

    // TAG
    if (ImGui::BeginCombo("Tag", groups[selectedGroupIndex].tag.c_str())) {
        for (int n = 0; n < IM_ARRAYSIZE(presetTags); n++) {
            bool is_selected = (groups[selectedGroupIndex].tag == presetTags[n]);
            if (ImGui::Selectable(presetTags[n], is_selected)) {
                if (multiEditMode) {
                    for (int idx : selectedGroups)
                        groups[idx].tag = presetTags[n];
                }
                else {
                    groups[selectedGroupIndex].tag = presetTags[n];
                }
            }
            if (is_selected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    // ✍️ TAG PERSONALIZZATO
    char tagBuffer[64];
    strncpy_s(tagBuffer, groups[selectedGroupIndex].tag.c_str(), sizeof(tagBuffer));
    if (ImGui::InputText("Tag Personalizzato", tagBuffer, sizeof(tagBuffer))) {
        if (multiEditMode) {
            for (int idx : selectedGroups)
                groups[idx].tag = tagBuffer;
        }
        else {
            groups[selectedGroupIndex].tag = tagBuffer;
        }
    }

    // VISUALIZZA COLORE TAG
    auto it = tagColors.find(groups[selectedGroupIndex].tag);
    if (it != tagColors.end()) {
        ImGui::TextColored(it->second, "Tag attivo: %s", groups[selectedGroupIndex].tag.c_str());
    }
    else {
        ImGui::Text("Tag attivo: %s", groups[selectedGroupIndex].tag.c_str());
    }

    // SALVATAGGIO ISTANTANEO
    if (!multiEditMode) {
        group.name = groupNameBuffer;
    }
    saveToFile(currentPath);

    if (ImGui::Button("Salva")) {
        saveToFile(currentPath);
    }
    ImGui::SameLine();
    if (ImGui::Button("Carica")) {
        loadFromFile(currentPath);
    }

    ImGui::Separator();
    ImGui::Text("Audio:");
    ImGui::InputText("Nome suono", group.soundName.data(), 64);
    ImGui::SliderFloat("Volume", &group.soundVolume, 0.f, 100.f);
    ImGui::Checkbox("Loop", &group.soundLoop);

    if (!group.soundName.empty()) {
        if (ImGui::Button("Testa suono")) {
            if (soundManager && soundManager->hasSound(group.soundName)) {
                soundManager->playSound(group.soundName, group.soundLoop, group.soundVolume);
            }
            else {
                std::cerr << "[Editor] Suono '" << group.soundName << "' non trovato!\n";
            }
        }
    }
}


void AnimationGroupEditorPanel::renderPreview() {
    static bool debugMode = false;
    static float timer = 0.0f;
    static bool playing = true;

    if (selectedGroupIndex < 0 || selectedGroupIndex >= groups.size()) return;

    const auto& group = groups[selectedGroupIndex];
    if (group.frames.empty()) return;

    const sf::Texture* texture = tileSetManager.getTexture(selectedTilesetName);
    if (!texture) return;

    if (playing) {
        timer += ImGui::GetIO().DeltaTime;
    }

    int frameIndex = static_cast<int>((timer / group.frameDuration)) % group.frames.size();
    const auto& frame = group.frames[frameIndex];

    ImGui::Checkbox("Modalita Debug Visivo", &debugMode);
    ImGui::Text("Anteprima:");
    ImGui::BeginChild("PreviewAnimata", ImVec2(200, 200), true);

    int columns = texture->getSize().x / tileWidth;
    for (const auto& tile : frame) {
        int tx = (tile.tileID % columns) * tileWidth;
        int ty = (tile.tileID / columns) * tileHeight;

        ImVec2 uv0(tx / (float)texture->getSize().x, ty / (float)texture->getSize().y);
        ImVec2 uv1((tx + tileWidth) / (float)texture->getSize().x, (ty + tileHeight) / (float)texture->getSize().y);

        ImVec2 pos(tile.offset.x * 48.f, tile.offset.y * 48.f);
        ImGui::SetCursorPos(pos);
        ImGui::Image((void*)(intptr_t)texture->getNativeHandle(), ImVec2(48, 48), uv0, uv1);

        // DEBUG VISIVO – solo se abilitato
        if (debugMode) {
            ImGui::GetWindowDrawList()->AddRect(
                ImGui::GetItemRectMin(),
                ImGui::GetItemRectMax(),
                IM_COL32(255, 255, 0, 180)); // Bordo giallo trasparente

            char label[32];
            snprintf(label, sizeof(label), "ID: %d\n(%d,%d)", tile.tileID, tile.offset.x, tile.offset.y);
            ImVec2 textPos = ImGui::GetItemRectMin();
            textPos.y -= 12;
            ImGui::GetWindowDrawList()->AddText(textPos, IM_COL32_WHITE, label);
        }
    }

    ImGui::EndChild();

    ImGui::Separator();
    if (ImGui::Button(playing ? "Pausa" : "Play")) {
        playing = !playing;
    }
    ImGui::SameLine();
    if (ImGui::Button("Avanza 1 frame") && !playing) {
        timer += group.frameDuration;
    }

    ImGui::Separator();
    ImGui::Text("Debug:");
    ImGui::Text("Frame attuale: %d / %d", frameIndex + 1, (int)group.frames.size());
    ImGui::Text("Tempo accumulato: %.2f s", timer);
    ImGui::Text("Durata per frame: %.2f s", group.frameDuration);
}





void AnimationGroupEditorPanel::setTileSize(int w, int h) {
    tileWidth = w;
    tileHeight = h;
}

void AnimationGroupEditorPanel::setCurrentTileID(int id) {
    currentTileID = id;
}

void AnimationGroupEditorPanel::setTilesetName(const std::string& name) {
    selectedTilesetName = name;
}

std::vector<AnimationGroup>& AnimationGroupEditorPanel::getGroups() {
    return groups;
}

void AnimationGroupEditorPanel::loadFromFile(const std::string& path) {
    std::ifstream in(path);
    if (!in.is_open()) {
        std::cerr << "[AnimationGroupEditor] Impossibile aprire " << path << "\n";
        return;
    }

    try {
        json j;
        in >> j;

        if (!j.is_object()) {
            std::cerr << "[AnimationGroupEditor] Errore: JSON non valido\n";
            return;
        }

        groups.clear();
        for (const auto& [name, groupJson] : j.items()) {
            if (name == "_placedInstances") continue;

            AnimationGroup g;
            g.name = name;
            g.frameDuration = groupJson.value("frameDuration", 0.2f);
            g.cols = groupJson.value("cols", 1);
            g.rows = groupJson.value("rows", 1);

            const auto& frameArray = groupJson["frames"];
            for (const auto& frameJson : frameArray) {
                std::vector<AnimationGroupFrame> frame;
                for (const auto& t : frameJson) {
                    AnimationGroupFrame f;
                    f.tileID = t["tileID"];
                    auto offset = t["offset"];
                    f.offset = sf::Vector2i(offset[0], offset[1]);
                    frame.push_back(f);
                }
                g.frames.push_back(frame);
            }

            groups.push_back(g);
        }

        // Placed instances
        if (j.contains("_placedInstances") && j["_placedInstances"].is_array()) {
            placedInstances.clear();
            for (const auto& inst : j["_placedInstances"]) {
                PlacedGroupInstance p;
                p.groupName = inst["groupName"];
                auto pos = inst["position"];
                p.position = sf::Vector2i(pos[0], pos[1]);
                placedInstances.push_back(p);
            }
        }
    }

    catch (const std::exception& e) {
        std::cerr << "[AnimationGroupEditor] Errore nel parsing di " << path << ": " << e.what() << "\n";

        // Se errore => resetta i gruppi
        groups.clear();
        std::ofstream reset(path);
        if (reset.is_open()) {
            reset << "{}";
            std::cout << "[AnimationGroupEditor] File " << path << " ripristinato vuoto.\n";
        }
    }
}


void AnimationGroupEditorPanel::saveToFile(const std::string& path) {
    json j;
    for (const auto& group : groups) {
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

    j["_placedInstances"] = json::array();
    for (const auto& instance : placedInstances) {
        j["_placedInstances"].push_back({
            {"groupName", instance.groupName},
            {"position", {instance.position.x, instance.position.y}}
            });
    }


    std::ofstream out(path);
    out << std::setw(4) << j;
}

void AnimationGroupEditorPanel::pushUndo() {
    undoStack.push_back(groups);     // usa il membro groups della classe
    redoStack.clear();               // cancella redo
}


void AnimationGroupEditorPanel::renderPlacedInstances() {
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    const sf::Texture* texture = tileSetManager.getTexture(selectedTilesetName);
    if (!texture) return;

    for (const auto& instance : placedInstances) {
        auto it = std::find_if(groups.begin(), groups.end(), [&](const AnimationGroup& g) {
            return g.name == instance.groupName;
            });

        if (it == groups.end() || it->frames.empty()) continue;

        const AnimationGroup& group = *it;
        int frameIndex = static_cast<int>((previewElapsed / group.frameDuration)) % group.frames.size();
        const auto& frame = group.frames[frameIndex];

        int columns = texture->getSize().x / tileWidth;

        for (const auto& tile : frame) {
            // Calcola la posizione assoluta del tile rispetto all'origine dell'istanza
            sf::Vector2i absPos = instance.position + sf::Vector2i(tile.offset.y, tile.offset.x); // NOTA L'ORDINE Y, X SE SERVE
            int tx = (tile.tileID % columns) * tileWidth;
            int ty = (tile.tileID / columns) * tileHeight;

            ImVec2 uv0(tx / (float)texture->getSize().x, ty / (float)texture->getSize().y);
            ImVec2 uv1((tx + tileWidth) / (float)texture->getSize().x, (ty + tileHeight) / (float)texture->getSize().y);

            ImVec2 screenPos(absPos.x * 48.f, absPos.y * 48.f);
            drawList->AddImage(
                (ImTextureID)(intptr_t)texture->getNativeHandle(),
                screenPos,
                ImVec2(screenPos.x + 48.f, screenPos.y + 48.f),
                uv0, uv1
            );
        }
    }

}

// CHIAMA questa funzione dentro render() PRIMA di ImGui::End():
// renderPlacedInstances();


void AnimationGroupEditorPanel::handleMapClick(sf::Vector2i gridPos) {
    if (placingInstanceMove && selectedPlacedInstance >= 0 && selectedPlacedInstance < placedInstances.size()) {
        placedInstances[selectedPlacedInstance].position = gridPos;
        placingInstanceMove = false;
    }
}

void AnimationGroupEditorPanel::renderPlacedInstancePanel() {
    ImGui::Begin("Istanze Piazzate");

    for (int i = 0; i < placedInstances.size(); ++i) {
        const auto& instance = placedInstances[i];

        char label[128];
        snprintf(label, sizeof(label), "%s @ (%d,%d)", instance.groupName.c_str(), instance.position.x, instance.position.y);

        if (ImGui::Selectable(label, selectedPlacedInstance == i)) {
            selectedPlacedInstance = i;
        }
    }

    if (selectedPlacedInstance >= 0 && selectedPlacedInstance < placedInstances.size()) {
        ImGui::Separator();
        auto& inst = placedInstances[selectedPlacedInstance];

        ImGui::Text("Posizione:");
        ImGui::InputInt("X", &inst.position.x);
        ImGui::InputInt("Y", &inst.position.y);

        if (ImGui::Button("Posiziona cliccando nella mappa")) {
            placingInstanceMove = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("Elimina")) {
            placedInstances.erase(placedInstances.begin() + selectedPlacedInstance);
            selectedPlacedInstance = -1;
        }
    }

    ImGui::End();
}

// ⚠️ Ricorda: nella gestione eventi del tuo editor (mouse click sulla mappa), chiama:
// handleMapClick(sf::Vector2i cliccato)
// dove cliccato è la cella griglia cliccata

void AnimationGroupEditorPanel::trySelectPlacedInstanceAt(sf::Vector2i gridPos) {
    for (int i = 0; i < placedInstances.size(); ++i) {
        if (placedInstances[i].position == gridPos) {
            selectedPlacedInstance = i;
            placingInstanceMove = false;
            break;
        }
    }
}




