#include "Game.h"
#include "MenuState.h"
#include "EditorState.h"
#include <iostream>
#include <imgui.h>
#include <imgui-SFML.h>
#include "WorldSettingsManager.h"

// DEFINIZIONE delle variabili globali
WorldSettingsManager* worldSettings = nullptr;
bool showWorldSettings = false;


Game::Game() {
    //std::cout << "[Game] Costruttore INIZIO\n";

    window = nullptr;  // inizializzato a nullptr

    zoomFactor = 1.0f;
    isFullscreen = false;

    // NON CREARE la finestra qui!
    // window->create(...) DA NON USARE!

    //updateView();

    //std::cout << "[Game] Costruttore completato\n";
}


void Game::init() {
    if (window) {
        updateView(); // ora è sicuro
    }
    else {
        std::cerr << "[Game::init] Errore: window non impostata!\n";
    }
}



void Game::run() {
    //std::cout << "[Game::run] Entro nel ciclo principale\n";

    sf::Clock deltaClock;
    while (window->isOpen()) {
        sf::Event event;
        while (window->pollEvent(event)) {
            ImGui::SFML::ProcessEvent(event);  // <-- AGGIUNGI QUESTA RIGA

            if (event.type == sf::Event::Closed)
                window->close();

            if (event.type == sf::Event::Resized)
                updateView();

            if (getCurrentState())
                getCurrentState()->handleInput(event);
        }

        float dt = deltaClock.restart().asSeconds();

        ImGui::SFML::Update(*window, sf::seconds(dt)); // <-- AGGIUNGI QUESTO

        if (getCurrentState())
            getCurrentState()->update(dt);


        if (auto* editor = dynamic_cast<EditorState*>(getCurrentState())) {
            window->clear(editor->getBackgroundColor());
        }
        else {
            window->clear(sf::Color::Black); // fallback
        }





        if (getCurrentState())
            getCurrentState()->render();

        ImGui::SFML::Render(*window);  // <-- ANCHE QUESTO

        window->display();
    }
}


void Game::pushState(std::unique_ptr<GameState> state) {
    states.push_back(std::move(state));
}

void Game::popState() {
    if (!states.empty()) states.pop_back();
}

void Game::changeState(std::unique_ptr<GameState> state) {
    if (!states.empty()) states.pop_back();
    states.push_back(std::move(state));
}

GameState* Game::getCurrentState() {
    if (states.empty()) return nullptr;
    return states.back().get();
}

GameState* Game::getPreviousState() {
    if (states.size() < 2) return nullptr;
    return states[states.size() - 2].get();
}

void Game::updateView() {
    gameView.setSize(window->getSize().x / zoomFactor, window->getSize().y / zoomFactor);
    gameView.setCenter(gameView.getSize().x / 2, gameView.getSize().y / 2);
    window->setView(gameView);
}

void Game::adjustZoom(float zoomFactor) {
    gameView.zoom(zoomFactor);
}



void Game::toggleFullscreen() {
    isFullscreen = !isFullscreen;
    sf::Vector2u oldSize = window->getSize();
    window->close();
    if (isFullscreen) {
        window->create(sf::VideoMode::getDesktopMode(), "Top-Down Game", sf::Style::Fullscreen);
    }
    else {
        window->create(sf::VideoMode(1280, 720), "Top-Down Game", sf::Style::Default);
    }
    updateView();
}

sf::View& Game::getGameView(){
    return gameView;  //restituisce una COPIA
}


void Game::setWindow(sf::RenderWindow* newWindow) {
    window = newWindow;
}

void Game::setBackgroundColor(const sf::Color& color) {
    backgroundColor = color;
}



void Game::setGameView(const sf::View& view) {
    gameView = view;
}

void Game::setViewCenterClamp(const sf::Vector2f& center, const sf::Vector2u& mapPixelSize) {
    sf::Vector2f halfSize = gameView.getSize() * 0.5f;

    sf::Vector2f clampedCenter = center;
    clampedCenter.x = std::clamp(clampedCenter.x, halfSize.x, static_cast<float>(mapPixelSize.x) - halfSize.x);
    clampedCenter.y = std::clamp(clampedCenter.y, halfSize.y, static_cast<float>(mapPixelSize.y) - halfSize.y);

    gameView.setCenter(clampedCenter);
    window->setView(gameView);
}
