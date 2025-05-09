#pragma once
#include <unordered_map>
#include <string>
#include <memory>
#include "Entity.h"
#include "NPC.h"
#include "GameObject.h"
#include "ScriptManager.h"

class GameWorld {
public:
    GameWorld();

    void update(float dt); // Tick generale
    void handleEvent(const std::string& event, const std::string& entityId); // es: "onClick", "onDeath"

    // Gestione entità
    void addEntity(std::shared_ptr<Entity> e);
    void removeEntity(const std::string& id);
    std::shared_ptr<Entity> getEntity(const std::string& id);
    void movePlayer(const std::string& playerId, int dx, int dy);

    // Script manager
    ScriptManager& getScriptManager();

    // Rendering se vuoi supportarlo direttamente (editor/single)
    void render(sf::RenderWindow& window);

private:
    std::unordered_map<std::string, std::shared_ptr<Entity>> entities;
    ScriptManager scriptManager;
};
