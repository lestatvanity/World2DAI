#include "InGameState.h"
#include "Game.h"
#include "PauseState.h"
#include "MenuState.h"
#include "EditorState.h"
#include <iostream>
InGameState::InGameState(Game& game) : GameState(game), gameWorld(), controller(gameWorld) {
    player.setRadius(40.f);
    player.setFillColor(sf::Color::Cyan);
    player.setPosition(300.f, 300.f);
}

void InGameState::handleInput(sf::Event& event) {
    if (event.type == sf::Event::KeyPressed) {
        if (event.key.code == sf::Keyboard::Space) {
            game.pushState(std::make_unique<PauseState>(game));
        }
        if (event.key.code == sf::Keyboard::F10) {
            try {
                game.changeState(std::make_unique<EditorState>(game));
            }
            catch (const std::exception& e) {
                std::cerr << "[Editor - Crash] " << e.what() << "\n";
            }
        }

        else if (event.key.code == sf::Keyboard::Escape) {
            game.changeState(std::make_unique<MenuState>(game));
        }
    }
}

void InGameState::update(float) {}

void InGameState::render() {
    game.getWindow().draw(player);
}