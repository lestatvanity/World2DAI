#pragma once
#include "GameState.h"

class PauseState : public GameState {
public:
    PauseState(Game& game);
    void handleInput(sf::Event& event) override;
    void update(float dt) override;
    void render() override;
};