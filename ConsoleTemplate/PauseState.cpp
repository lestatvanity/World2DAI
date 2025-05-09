#include "PauseState.h"
#include "Game.h"

PauseState::PauseState(Game& game) : GameState(game) {}

void PauseState::handleInput(sf::Event& event) {
    if (event.type == sf::Event::KeyPressed) {
        if (event.key.code == sf::Keyboard::Space || event.key.code == sf::Keyboard::Escape) {
            game.popState();
        }
    }
}

void PauseState::update(float) {}

void PauseState::render() {
    GameState* under = game.getPreviousState();
    if (under) {
        under->render(); // disegna il gioco sotto la pausa
    }

    sf::Font font;
    if (font.loadFromFile("assets/arial.ttf")) {
        sf::Text text("PAUSA", font, 40);
        text.setFillColor(sf::Color::Yellow);
        text.setPosition(game.getGameView()
            .getCenter().x - 60, game.getGameView()
            .getCenter().y - 30);
        game.getWindow().draw(text);
    }
}
