#include "imgui_includes.h"
#include "PropertyPanel.h"
#include "Entity.h"
#include <iostream>
#include <sstream>
#include <fstream>


PropertyPanel::PropertyPanel()
    : showEventEditor(false) {
}

void PropertyPanel::setSelectedEntity(const SelectedEntity& sel) {
    selected = sel;
}

void PropertyPanel::update() {
    // Nulla per ora
}

void PropertyPanel::render(sf::RenderWindow& window) {
     // Funzione VECCHIA: usata solo se vuoi ancora disegnare testo su SFML
     // Non serve più nulla qui se usi solo ImGui
}

void PropertyPanel::renderImGui() {
    ImGui::Begin("Proprieta Entita");

    if (selected.type == SelectedEntity::Type::None) {
        ImGui::Text("Nessuna selezione attiva.");
    }
    else {
        // --- Info base ---
        if (selected.type == SelectedEntity::Type::Tile) {
            ImGui::Text("Tipo: Tile");
            ImGui::Separator();
            ImGui::Text("Tile ID: %d", selected.tile.tileID);
            ImGui::Text("Tileset: %s", selected.tile.tilesetName.c_str());
            ImGui::Text("Posizione: (%d, %d)", selected.tilePosition.x, selected.tilePosition.y);
        }
        else if (selected.type == SelectedEntity::Type::Group) {
            ImGui::Text("Tipo: Gruppo Animato");
            ImGui::Separator();
            ImGui::Text("Nome Gruppo: %s", selected.groupName.c_str());
            ImGui::Text("Posizione: (%d, %d)", selected.groupPosition.x, selected.groupPosition.y);
            ImGui::Text("Layer: %d", selected.groupLayer);
        }
        else if (selected.type == SelectedEntity::Type::Entity && selected.entity) {
            ImGui::Text("Tipo: Entità");
            ImGui::Separator();
            ImGui::Text("ID: %s", selected.entity->getID().c_str());
            ImGui::Text("Tipo: %s", selected.entity->getType().c_str());
            ImGui::Text("Posizione: (%d, %d)", selected.entity->getX(), selected.entity->getY());
            ImGui::Text("Animazione Corrente: %s", selected.entity->getCurrentAnimation().c_str());
        }

        ImGui::Spacing();

        // --- Bottone Aggiungi Evento ---
        if (ImGui::Button("Aggiungi Evento")) {
            if (selected.type != SelectedEntity::Type::None) {
                std::string entityType;
                if (selected.type == SelectedEntity::Type::Tile) entityType = "tile";
                else if (selected.type == SelectedEntity::Type::Group) entityType = "group";
                else if (selected.type == SelectedEntity::Type::Entity) entityType = "entity";
                else entityType = "area"; // futuro


                availableEvents = scriptManager->getAvailableEventsForType(entityType);
                showEventEditor = true;
            }
        }
    }

    ImGui::End();

    // --- Finestra Editor Evento ---
    if (showEventEditor) {
        ImGui::Begin("Editor Evento", &showEventEditor);

        static char scriptPath[256] = "";
        static int selectedEventIndex = 0;

        if (!availableEvents.empty()) {
            std::vector<const char*> eventLabels;
            for (const auto& ev : availableEvents) {
                eventLabels.push_back(ev.c_str());
            }

            ImGui::Combo("Tipo Evento", &selectedEventIndex, eventLabels.data(), eventLabels.size());
        }
        else {
            ImGui::Text("Nessun evento disponibile.");
        }

        ImGui::InputText("Percorso Script Lua", scriptPath, IM_ARRAYSIZE(scriptPath));

        if (ImGui::Button("Salva Evento")) {
            if (!availableEvents.empty()) {
                AssignedEvent ev;
                if (selected.type == SelectedEntity::Type::Tile) {
                    ev.targetType = "Tile";
                    ev.position = selected.tilePosition;
                }
                else if (selected.type == SelectedEntity::Type::Group) {
                    ev.targetType = "Group";
                    ev.groupName = selected.groupName;
                    ev.position = selected.groupPosition;
                }
                else if (selected.type == SelectedEntity::Type::Entity && selected.entity) {
                    ev.targetType = "Entity";
                    ev.entityID = selected.entity->getID();
                }

                ev.eventType = availableEvents[selectedEventIndex];
                ev.scriptPath = scriptPath;

                events.push_back(ev);

                // --- Creazione File Lua ---
                std::ofstream luaFile("scripts/" + std::string(scriptPath) + ".lua");
                if (luaFile.is_open()) {
                    luaFile << "function " << ev.eventType << "(entity)\n";
                    luaFile << "    -- TODO: Logica dell'evento\n";
                    luaFile << "end\n";
                    luaFile.close();
                }

                showEventEditor = false;
                memset(scriptPath, 0, sizeof(scriptPath));
                selectedEventIndex = 0;
            }
        }

        ImGui::SameLine();

        if (ImGui::Button("Annulla")) {
            showEventEditor = false;
        }

        ImGui::End();
    }
}


void PropertyPanel::renderEventEditor() {
    ImGui::Begin("Editor Evento", &showEventEditor);

    static char eventType[128] = "";
    static char scriptPath[256] = "";

    static int selectedEventIndex = 0;

    if (!availableEvents.empty()) {
        std::vector<const char*> eventNames;
        for (const auto& e : availableEvents)
            eventNames.push_back(e.c_str());

        if (ImGui::Combo("Tipo Evento", &selectedEventIndex, eventNames.data(), eventNames.size())) {
            strcpy_s(eventType, availableEvents[selectedEventIndex].c_str());
        }
    }
    else {
        ImGui::Text("Nessun evento disponibile!");
    }

    ImGui::InputText("Percorso Script Lua", scriptPath, IM_ARRAYSIZE(scriptPath));

    if (ImGui::Button("Salva Evento")) {
        if (selected.type != SelectedEntity::Type::None) {
            EventData newEvent;
            newEvent.eventType = eventType;
            newEvent.scriptPath = scriptPath;

            selected.events.push_back(newEvent);

            std::cout << "[EventEditor] Evento aggiunto: " << newEvent.eventType << " -> " << newEvent.scriptPath << "\n";
        }

        // Reset campi
        eventType[0] = '\0';
        scriptPath[0] = '\0';
        showEventEditor = false;
    }

    ImGui::SameLine();
    if (ImGui::Button("Annulla")) {
        showEventEditor = false;
    }

    ImGui::End();
}

void PropertyPanel::saveEventsToFile(const std::string& path) {
    nlohmann::json j;

    for (const auto& ev : events) {
        j.push_back({
            {"targetType", ev.targetType},
            {"position", { ev.position.x, ev.position.y }},
            {"groupName", ev.groupName},
            {"entityID", ev.entityID},
            {"eventType", ev.eventType},
            {"scriptPath", ev.scriptPath}
            });
    }

    std::ofstream out(path);
    if (out.is_open()) {
        out << std::setw(4) << j;
        std::cout << "[PropertyPanel] Eventi salvati in " << path << "\n";
    }
    else {
        std::cerr << "[PropertyPanel] Errore salvataggio eventi in " << path << "\n";
    }
}

void PropertyPanel::loadEventsFromFile(const std::string& path) {
    std::ifstream in(path);
    if (!in.is_open()) {
        std::cerr << "[PropertyPanel] File eventi non trovato: " << path << "\n";
        return;
    }

    nlohmann::json j;
    in >> j;

    events.clear();

    for (const auto& item : j) {
        AssignedEvent ev;
        ev.targetType = item.value("targetType", "");
        auto pos = item.value("position", std::vector<int>{0, 0});
        if (pos.size() == 2) {
            ev.position = { pos[0], pos[1] };
        }
        ev.groupName = item.value("groupName", "");
        ev.entityID = item.value("entityID", "");
        ev.eventType = item.value("eventType", "");
        ev.scriptPath = item.value("scriptPath", "");

        events.push_back(ev);
    }

    std::cout << "[PropertyPanel] Caricati " << events.size() << " eventi da " << path << "\n";
}
