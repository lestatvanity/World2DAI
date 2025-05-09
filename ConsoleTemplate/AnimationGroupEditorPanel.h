// AnimationGroupEditorPanel.h
#pragma once

#include "TileSetManager.h"  // Gestione dei tileset per accedere alle texture
#include "AnimationGroup.h"   // Definizione delle strutture dati per i gruppi animati
#include "SoundManager.h"     // Per collegare suoni ai gruppi animati
#include <string>
#include <vector>
#include <imgui.h>            // UI interattiva con ImGui
#include <SFML/Graphics.hpp>  // Supporto grafico per preview

struct PlacedGroupInstance {
    std::string groupName;       // Nome del gruppo (deve corrispondere a uno esistente in groups)
    sf::Vector2i position;       // Posizione nella mappa (es. griglia o pixel)
};


// Pannello per gestire gruppi di tile animati nel tileset
class AnimationGroupEditorPanel {
public:
    AnimationGroupEditorPanel(TileSetManager& manager);                            // Costruttore che riceve il riferimento al gestore tileset
    void render(bool* open, bool& placingGroup, std::string& currentGroupToPlace); // Disegna il pannello e gestisce input utente
    void setTileSize(int w, int h);                                                // Imposta dimensioni dei tile
    void setCurrentTileID(int id);                                                 // Imposta il tile ID selezionato
    void setTilesetName(const std::string& name);                                  // Imposta il nome del tileset attivo
    void setCurrentPath(const std::string& path) {
        currentPath = path;
    }
    std::vector<AnimationGroup>& getGroups();                                      // Ritorna riferimento ai gruppi animati
    void loadFromFile(const std::string& path);                                    // Carica i gruppi animati da file JSON
    void saveToFile(const std::string& path);                                      // Salva i gruppi animati su file JSON
    void setSoundManager(SoundManager* manager) { soundManager = manager; }        // Collegamento al gestore dei suoni
    std::vector<PlacedGroupInstance> placedInstances;                              // tutte le istanze visibili nella mappa
    void renderPlacedInstances();
    void handleMapClick(sf::Vector2i gridPos);
    void renderPlacedInstancePanel();
    void trySelectPlacedInstanceAt(sf::Vector2i gridPos);
    std::function<void(const std::string& groupName, sf::Vector2i position)> onGroupPlaced;

private:
    SoundManager* soundManager = nullptr;         // Puntatore al gestore suoni, può essere nullo
    void renderSidebar(bool& placingGroup, std::string& currentGroupToPlace); // Sidebar per selezione e creazione gruppi
    void renderGridComposer();                    // Compositore visivo per posizionare frame su griglia
    void renderGroupProperties();                 // Mostra proprietà del gruppo selezionato
    void renderPreview();                         // Anteprima dell'animazione costruita

    TileSetManager& tileSetManager;               // Riferimento al gestore dei tileset
    std::vector<AnimationGroup> groups;           // Collezione di gruppi animati

    int tileWidth = 32;                           // Larghezza tile in pixel
    int tileHeight = 32;                          // Altezza tile in pixel
    int selectedGroupIndex = -1;                  // Indice del gruppo attualmente selezionato
    int selectedFrame = 0;                        // Frame selezionato per modifica o preview
    int currentTileID = -1;                       // ID del tile attualmente selezionato
    int hoveredCellX = -1, hoveredCellY = -1;     // Coordinate della cella su cui è posizionato il mouse

    std::string selectedTilesetName;              // Nome del tileset attivo
    char groupNameBuffer[64] = "";                // Buffer per l'inserimento del nome del gruppo
    float frameDuration = 0.2f;                   // Durata default di un frame nel gruppo
    int cols = 3, rows = 3;                       // Dimensioni griglia del compositore

    std::string currentPath; // Percorso di default per salvataggio/caricamento

    // Undo/Redo per modifiche ai gruppi animati
    std::vector<std::vector<AnimationGroup>> undoStack;
    std::vector<std::vector<AnimationGroup>> redoStack;

    // Metodo per salvare lo stato corrente nello stack undo
    void pushUndo();

    std::set<int> selectedGroups; // Indici dei gruppi selezionati
    bool multiEditMode = false;   // Flag modalità multipla
   


};

