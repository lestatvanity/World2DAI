#pragma once
#include <SFML/Graphics.hpp>

class Game;

class GameState {
public:
    GameState(Game& game) : game(game) {}
    virtual ~GameState() = default;

    virtual void handleInput(sf::Event& event) = 0;
    virtual void update(float dt) = 0;
    virtual void render() = 0;

protected:
    Game& game;
};
