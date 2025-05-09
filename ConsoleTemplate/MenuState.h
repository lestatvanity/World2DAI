#pragma once
#include "GameState.h"

class MenuState : public GameState {
public:
    MenuState(Game& game);
    void handleInput(sf::Event& event) override;
    void update(float dt) override;
    void render() override;
};