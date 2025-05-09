// WorldSettingsManager.h
#pragma once
//#include "Game.h"
#include <string>
#include <SFML/Graphics.hpp>

struct WorldConfig;
class Game;

class WorldSettingsManager {
public:
    WorldSettingsManager(WorldConfig& config, Game& game);


    void renderPanel(bool* open); // Usa questo per aprire/chiudere il pannello
    sf::Color getBackgroundColor() const;
    const WorldConfig& getConfig() const { return config; }


private:
    WorldConfig& config;
    Game& game;
    float bgColor[3]; // RGB da 0 a 1
};

class WorldSettingsManager;
// Variabili globali: DICHIARAZIONE (non definizione!)
//extern WorldSettingsManager* worldSettings;
//extern bool showWorldSettings;