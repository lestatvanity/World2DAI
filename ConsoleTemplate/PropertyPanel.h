#pragma once
#include "TileMap.h"
#include "AnimationGroup.h"
#include "ScriptManager.h"
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>

class Entity;

class ScriptManager;


struct AssignedEvent {
    std::string targetType;  // "Tile", "Group" o "Entity"
    sf::Vector2i position;   // per Tile e Group
    std::string groupName;   // per Group
    std::string entityID;    // per Entity
    std::string eventType;
    std::string scriptPath;
};


struct EventData {
    std::string eventType;
    std::string scriptPath;
};

struct SelectedEntity {
    enum class Type { None, Tile, Group, Entity } type = Type::None;
    TileInstance tile;
    sf::Vector2i tilePosition;      // posizione sulla mappa
    std::string groupName;          // nome del gruppo (stringa)
    sf::Vector2i groupPosition;     // posizione sulla mappa
    int groupLayer = 0;             // layer sulla mappa
    Entity* entity = nullptr;       // puntatore all'entità
    std::vector<EventData> events;  // <<< aggiunto, lista eventi!
};

class PropertyPanel {
public:
    PropertyPanel();

    void setSelectedEntity(const SelectedEntity& sel);
    void update();
    void render(sf::RenderWindow& window);  // SOLO sfml base
    void renderImGui();                     // <<<<<< nuovo per ImGui moderno
    void saveEventsToFile(const std::string& path);
    void loadEventsFromFile(const std::string& path);

    void setScriptManager(ScriptManager* manager) { scriptManager = manager; } // <<<<<<<<<< nuovo


private:
    SelectedEntity selected;
    ScriptManager* scriptManager = nullptr; // <<<<<<<<<<<<<< nuovo


    sf::Font font;
    sf::Text idText;
    sf::Text positionText;
    sf::Text typeText;
    sf::Text animationText;
    sf::RectangleShape background;

    bool showEventEditor = false; // <<< finestra eventi Lua
    void renderEventEditor();     // <<< funzione privata
    std::vector<AssignedEvent> events; // <<<<<< QUI events adesso è DEFINITO
    std::vector<std::string> availableEvents;


};
