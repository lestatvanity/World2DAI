#include "GameWorld.h"
#include "Entity.h"
#include <iostream>

extern std::unordered_map<std::string, Entity*> allEntities;


GameWorld::GameWorld()
    : scriptManager("scripts") {
    // eventuale caricamento mappe, NPC, script ecc.
}

void GameWorld::update(float dt) {
    for (auto& [id, e] : entities) {
        e->update(dt);
        scriptManager.onUpdate(id);  // Tick sugli script Lua
    }
}

void GameWorld::handleEvent(const std::string& event, const std::string& entityId) {
    if (event == "onClick")       scriptManager.onClick(entityId);
    else if (event == "onDeath")  scriptManager.onDeath(entityId);
    else if (event == "onInteract") scriptManager.onInteract(entityId);
    // estendibile
}

void GameWorld::addEntity(std::shared_ptr<Entity> e) {
    entities[e->getID()] = e;
}

void GameWorld::removeEntity(const std::string& id) {
    entities.erase(id);
}

std::shared_ptr<Entity> GameWorld::getEntity(const std::string& id) {
    if (entities.count(id)) return entities[id];
    return nullptr;
}

ScriptManager& GameWorld::getScriptManager() {
    return scriptManager;
}

void GameWorld::render(sf::RenderWindow& window) {
    for (auto& [_, e] : entities)
        e->render(window);
}


void GameWorld::movePlayer(const std::string& playerId, int dx, int dy) {
    auto it = allEntities.find(playerId);
    if (it != allEntities.end()) {
        Entity* entity = it->second;
        entity->moveBy(dx, dy);
        // eventuale log o controllo futuro
        std::cout << "[GameWorld] " << playerId << " si è mosso di (" << dx << "," << dy << ")\n";
    }
    else {
        std::cerr << "[GameWorld] Entity '" << playerId << "' non trovata!\n";
    }
}