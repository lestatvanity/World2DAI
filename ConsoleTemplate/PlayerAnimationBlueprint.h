#pragma once
#include <string>

struct PlayerAnimationBlueprint {
    std::string stateName;         // Es: "idle", "walk", "run"
    std::string texturePath;       // Es: "Idle.png"
    int frameWidth = 0;            // Dimensione orizzontale del frame
    int frameHeight = 0;           // Dimensione verticale del frame
    int rowCount = 1;              // Numero di righe nella spritesheet
    int columnCount = 1;           // Numero di colonne nella spritesheet
    float frameTime = 0.1f;        // Tempo per frame
    bool loop = true;              // Se l'animazione si ripete
    bool directional = false;      // True se ha 4 direzioni (una per riga)

    PlayerAnimationBlueprint() = default;

    PlayerAnimationBlueprint(
        const std::string& state,
        const std::string& path,
        int fWidth,
        int fHeight,
        int rows,
        int cols,
        float time,
        bool loopAnim = true,
        bool isDirectional = false
    ) : stateName(state), texturePath(path), frameWidth(fWidth), frameHeight(fHeight),
        rowCount(rows), columnCount(cols), frameTime(time), loop(loopAnim), directional(isDirectional) {}
};
