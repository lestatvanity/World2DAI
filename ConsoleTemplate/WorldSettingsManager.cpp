// WorldSettingsManager.cpp
#include "WorldSettingsManager.h"
#include "WorldConfig.h"
#include "imgui.h"
#include "Game.h"
#include <iostream>

WorldSettingsManager::WorldSettingsManager(WorldConfig& cfg, Game& gm)
    : config(cfg), game(gm) {
    bgColor[0] = config.backgroundColor.r / 255.f;
    bgColor[1] = config.backgroundColor.g / 255.f;
    bgColor[2] = config.backgroundColor.b / 255.f;

}

void WorldSettingsManager::renderPanel(bool* open) {
    //std::cout << "Render pannello WorldSettings\n";

    if (!ImGui::Begin("Proprieta del Mondo", open)) {
        ImGui::End();
        return;
    }

    // Mostra editor colore sfondo
    if (ImGui::ColorEdit3("Colore sfondo", bgColor)) {
        config.backgroundColor = sf::Color(
            static_cast<sf::Uint8>(bgColor[0] * 255),
            static_cast<sf::Uint8>(bgColor[1] * 255),
            static_cast<sf::Uint8>(bgColor[2] * 255)
        );
    }

    ImGui::Separator();
    ImGui::Text("Dimensioni mappa:");
    ImGui::InputInt("Larghezza (tile)", &config.mapWidth);
    ImGui::InputInt("Altezza (tile)", &config.mapHeight);

    ImGui::Separator();
    ImGui::Text("Dimensioni Tile:");
    ImGui::InputInt("Larghezza px", &config.tileWidth);
    ImGui::InputInt("Altezza px", &config.tileHeight);
    ImGui::InputInt("Dimensione su schermo", &config.tileScreenSize);

    ImGui::Separator();
    ImGui::InputText("Tileset Path", config.tilesetPath.data(), config.tilesetPath.size());
    ImGui::InputText("Metadata Path", config.metadataPath.data(), config.metadataPath.size());

    ImGui::Spacing();
    if (ImGui::Button("Salva modifiche in world.ini")) {
        saveWorldConfig("assets/world.ini", config);  // <-- salva!
        //std::cout << "[WorldSettingsManager] Salvato world.ini con nuove impostazioni!\n";
    }

    if (ImGui::Button("Chiudi")) {
        *open = false;
    }

    ImGui::End();
}


sf::Color WorldSettingsManager::getBackgroundColor() const {
    return sf::Color(
        static_cast<sf::Uint8>(bgColor[0] * 255),
        static_cast<sf::Uint8>(bgColor[1] * 255),
        static_cast<sf::Uint8>(bgColor[2] * 255)
    );
}
