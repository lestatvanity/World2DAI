// ──────────────────────────────────────────────────────────────
//  EventSystem.h  –  interfaccia del sistema eventi
//  Diviso dall'implementazione (vedi EventSystem.cpp)
// ──────────────────────────────────────────────────────────────

#pragma once

#include <SFML/System/Vector2.hpp>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace events {

    //--------------------------------------------------------------------
    // ENUM & STRUCT
    //--------------------------------------------------------------------

    enum class TargetType { Tile, Group, Entity };

    struct EventData {
        std::string      eventType;               // es. "onClick"
        std::string      scriptPath;              // percorso file .lua
        nlohmann::json   parameters = nullptr;    // parametri facoltativi
        bool             repeatable = true;      // once / repeat
    };

    struct AssignedEvent {
        TargetType       targetType = TargetType::Tile;

        //  Target specifics
        int              layer = 0;               // per Tile
        sf::Vector2i     position{ -1, -1 };     // per Tile & Group
        std::string      groupName;               // per Group
        std::string      entityID;                // per Entity

        EventData        data;                    // info script
    };

    //--------------------------------------------------------------------
    // Serializzazione JSON – dichiarazioni
    //--------------------------------------------------------------------

    void to_json(nlohmann::json& j, const sf::Vector2i& v);
    void from_json(const nlohmann::json& j, sf::Vector2i& v);

    void to_json(nlohmann::json& j, const EventData& e);
    void from_json(const nlohmann::json& j, EventData& e);

    void to_json(nlohmann::json& j, const AssignedEvent& ev);
    void from_json(const nlohmann::json& j, AssignedEvent& ev);

    //--------------------------------------------------------------------
    // Classe EventSystem – collezione e I/O
    //--------------------------------------------------------------------

    class EventSystem {
    public:
        bool loadFromFile(const std::string& path);
        bool saveToFile(const std::string& path) const;

        void addTileEvent(int layer, sf::Vector2i pos, const EventData& data);
        void addGroupEvent(const std::string& groupName, sf::Vector2i origin, const EventData& data);
        void addEntityEvent(const std::string& entityID, const EventData& data);

        std::vector<AssignedEvent>& getEvents();
        const std::vector<AssignedEvent>& getEvents() const;

    private:
        std::vector<AssignedEvent> events;
    };

} // namespace events
