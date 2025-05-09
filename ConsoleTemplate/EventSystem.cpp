// ──────────────────────────────────────────────────────────────
//  EventSystem.cpp   –   implementazione del sistema eventi
// ──────────────────────────────────────────────────────────────

#include "EventSystem.h"
#include <fstream>
#include <iostream>
#include <iomanip>

namespace events {

    //--------------------------------------------------------------------
    //  Free functions to_json / from_json (ADL‑visible)
    //--------------------------------------------------------------------

    void to_json(nlohmann::json& j, const sf::Vector2i& v) { j = nlohmann::json{ v.x, v.y }; }
    void from_json(const nlohmann::json& j, sf::Vector2i& v) { v.x = j.at(0).get<int>(); v.y = j.at(1).get<int>(); }

    void to_json(nlohmann::json& j, const EventData& e) {
        j = nlohmann::json{
            {"eventType",  e.eventType},
            {"scriptPath", e.scriptPath},
            {"parameters", e.parameters},
            {"repeatable", e.repeatable}
        };
    }
    void from_json(const nlohmann::json& j, EventData& e) {
        j.at("eventType").get_to(e.eventType);
        j.at("scriptPath").get_to(e.scriptPath);
        if (j.contains("parameters")) j.at("parameters").get_to(e.parameters);
        e.repeatable = j.value("repeatable", true);
    }

    //--------------------------------------------------------------------
    //  Helpers privati per AssignedEvent
    //--------------------------------------------------------------------
    static nlohmann::json assignedEventToJson(const AssignedEvent& ev)
    {
        switch (ev.targetType)
        {
        case TargetType::Tile:
            return {
                {"targetType", "Tile"},
                {"layer",      ev.layer},
                {"position",   nlohmann::json{ ev.position.x, ev.position.y }},
                {"data",       ev.data}
            };
        case TargetType::Group:
            return {
                {"targetType", "Group"},
                {"groupName",  ev.groupName},
                {"position",   nlohmann::json{ ev.position.x, ev.position.y }},
                {"data",       ev.data}
            };
        case TargetType::Entity:
        default:
            return {
                {"targetType", "Entity"},
                {"entityID",   ev.entityID},
                {"data",       ev.data}
            };
        }
    }

    static AssignedEvent jsonToAssignedEvent(const nlohmann::json& j)
    {
        AssignedEvent ev;
        std::string t = j.at("targetType");
        ev.targetType = (t == "Tile" ? TargetType::Tile : (t == "Group" ? TargetType::Group : TargetType::Entity));
        ev.layer = j.value("layer", 0);

        if (j.contains("position"))
        {
            const auto& arr = j["position"];
            if (arr.is_array() && arr.size() == 2)
                ev.position = { arr[0].get<int>(), arr[1].get<int>() };
        }
        ev.groupName = j.value("groupName", "");
        ev.entityID = j.value("entityID", "");
        j.at("data").get_to(ev.data);
        return ev;
    }

    void to_json(nlohmann::json& j, const AssignedEvent& ev) { j = assignedEventToJson(ev); }
    void from_json(const nlohmann::json& j, AssignedEvent& ev) { ev = jsonToAssignedEvent(j); }

    //--------------------------------------------------------------------
    //  EventSystem methods
    //--------------------------------------------------------------------

    bool EventSystem::loadFromFile(const std::string& path)
    {
        std::ifstream in(path);
        if (!in.is_open())
        {
            std::cerr << "[EventSystem] Impossibile aprire " << path << "\n";
            return false;
        }

        try
        {
            nlohmann::json j; in >> j; events.clear();
            for (const auto& sec : { "tile", "group", "entity" })
                if (j.contains(sec))
                    for (const auto& itm : j[sec])
                        events.push_back(jsonToAssignedEvent(itm));

            std::cout << "[EventSystem] Caricati " << events.size() << " eventi da " << path << "\n";
            return true;
        }
        catch (const std::exception& e)
        {
            std::cerr << "[EventSystem] Errore parsing " << path << ": " << e.what() << "\n";
            return false;
        }
    }

    bool EventSystem::saveToFile(const std::string& path) const
    {
        nlohmann::json root;
        for (const auto& ev : events)
        {
            switch (ev.targetType)
            {
            case TargetType::Tile:   root["tile"].push_back(assignedEventToJson(ev)); break;
            case TargetType::Group:  root["group"].push_back(assignedEventToJson(ev)); break;
            case TargetType::Entity: root["entity"].push_back(assignedEventToJson(ev)); break;
            }
        }

        std::ofstream out(path);
        if (!out.is_open())
        {
            std::cerr << "[EventSystem] Impossibile scrivere " << path << "\n";
            return false;
        }
        out << std::setw(4) << root;
        std::cout << "[EventSystem] Salvati " << events.size() << " eventi in " << path << "\n";
        return true;
    }

    void EventSystem::addTileEvent(int layer, sf::Vector2i pos, const EventData& data)
    {
        events.push_back({ TargetType::Tile, layer, pos, "", "", data });
    }

    void EventSystem::addGroupEvent(const std::string& group, sf::Vector2i origin, const EventData& data)
    {
        events.push_back({ TargetType::Group, 0, origin, group, "", data });
    }

    void EventSystem::addEntityEvent(const std::string& id, const EventData& data)
    {
        events.push_back({ TargetType::Entity, 0, { -1,-1 }, "", id, data });
    }

    std::vector<AssignedEvent>& EventSystem::getEvents() { return events; }
    const std::vector<AssignedEvent>& EventSystem::getEvents() const { return events; }

} // namespace events
