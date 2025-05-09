#include "MenuState.h"
#include "Game.h"
#include "InGameState.h"

MenuState::MenuState(Game& game) : GameState(game) {}

void MenuState::handleInput(sf::Event& event) {
    if (event.type == sf::Event::KeyPressed) {
        if (event.key.code == sf::Keyboard::Enter) {
            game.changeState(std::make_unique<InGameState>(game));
        }
        else if (event.key.code == sf::Keyboard::Escape) {
            game.getWindow().close();  // *BOOM* chiude il gioco
        }
    }
}

void MenuState::update(float) {}

void MenuState::render() {
    sf::Font font;
    if (font.loadFromFile("assets/arial.ttf")) {
        sf::Text text("Premi INVIO per iniziare", font, 30);
        text.setFillColor(sf::Color::White);
        text.setPosition(game.getGameView()
            .getCenter().x - 150, game.getGameView()
            .getCenter().y - 20);
        game.getWindow().draw(text);
    }
}
