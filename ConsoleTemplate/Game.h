#pragma once
#include <SFML/Graphics.hpp>
#include <memory>
#include <vector>
#include "GameState.h"
#include "WorldSettingsManager.h"
class Game {
public:
    Game();
    void run();
    void pushState(std::unique_ptr<GameState> state);
    void popState();
    void changeState(std::unique_ptr<GameState> state);
    GameState* getPreviousState();
    GameState* getCurrentState();
    void setWindow(sf::RenderWindow* newWindow); // con asterisco
    void init();
    void updateView();
    void adjustZoom(float delta);
    void toggleFullscreen();
    sf::View& getGameView();
    void setBackgroundColor(const sf::Color& color);

    void setViewCenterClamp(const sf::Vector2f& center, const sf::Vector2u& mapPixelSize);

    void setGameView(const sf::View& view);
    //sf::View& getGameView();
    //void setGameView(const sf::View& view) { gameView = view; }

    //void updateView();
    
    sf::RenderWindow& getWindow() {
        if (!window) {
            throw std::runtime_error("[Game] Errore: finestra non inizializzata!");
        }
        return *window;
    }
    sf::Color backgroundColor = sf::Color::Transparent;
    

private:

    std::vector<std::unique_ptr<GameState>> states;
    const int tileSize = 16;
    sf::RenderWindow* window = nullptr;
    sf::View gameView;
    sf::View defaultView;
    sf::Vector2f originalViewSize;
    float currentZoom = 1.0f;

    


    float zoomFactor = 2.0f;
    bool isFullscreen = false;
};