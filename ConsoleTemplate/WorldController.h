#pragma once
#include "GameWorld.h"
#include <SFML/Window/Event.hpp>

class WorldController {
public:
    WorldController(GameWorld& world);

    void update(float dt);
    void handleInput(const sf::Event& event); // solo client/single player
    void simulate();                          // solo server/single player

private:
    GameWorld& world;
};
