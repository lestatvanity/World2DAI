#pragma once
#include "GameState.h"
#include "GameWorld.h"
#include "WorldController.h"

class InGameState : public GameState {
    GameWorld gameWorld;
    WorldController controller;


public:

    

    InGameState(Game& game); // ← solo dichiarazione!
    void handleInput(sf::Event& event) override;
    void update(float dt) override;
    void render() override;

private:
    sf::CircleShape player;
};
