#include "EntityPropertyPanel.h"
#include "Entity.h"
#include <cstring>

void EntityPropertyPanel::setEntity(Entity* entity) {
    selectedEntity = entity;
}

void EntityPropertyPanel::update() {
    // Se vuoi logica interna futura
}

void EntityPropertyPanel::render(sf::RenderWindow& window) {
    if (!selectedEntity) {
        ImGui::Text("Nessuna entità selezionata.");
        return;
    }

    ImGui::Text("ID: %s", selectedEntity->getID().c_str());

    static char name[128];
    strncpy_s(name, sizeof(name), selectedEntity->getName().c_str(), _TRUNCATE);
    if (ImGui::InputText("Nome", name, IM_ARRAYSIZE(name))) {
        selectedEntity->setName(name);
    }

    static char type[128];
    strncpy_s(type, sizeof(type), selectedEntity->getType().c_str(), _TRUNCATE);
    if (ImGui::InputText("Tipo", type, IM_ARRAYSIZE(type))) {
        selectedEntity->setType(type);
    }

    int x = selectedEntity->getX();
    int y = selectedEntity->getY();
    if (ImGui::InputInt("Posizione X", &x)) selectedEntity->setX(x);
    if (ImGui::InputInt("Posizione Y", &y)) selectedEntity->setY(y);

    static char script[128];
    strncpy_s(script, sizeof(script), selectedEntity->getScript().c_str(), _TRUNCATE);
    if (ImGui::InputText("Script", script, IM_ARRAYSIZE(script))) {
        selectedEntity->setScript(script);
    }

    bool active = selectedEntity->isActive();
    if (ImGui::Checkbox("Attivo", &active)) {
        selectedEntity->setActive(active);
    }
}
