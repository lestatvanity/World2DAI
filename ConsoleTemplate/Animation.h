#pragma once
#include <string>
#include <vector>
#include <fstream>

// Struttura che rappresenta un singolo frame di animazione con posizione nel tileset e durata in millisecondi
struct AnimationFrame {
    int textureX, textureY; // Coordinate del frame sul tileset
    int durationMs;         // Durata del frame in millisecondi
};

// Classe che gestisce una sequenza di AnimationFrame per creare un'animazione
class Animation {
public:
    Animation() = default; // Costruttore di default
    Animation(const std::string& name); // Costruttore con nome

    void addFrame(int x, int y, int durationMs);           // Aggiunge un nuovo frame all'animazione
    const AnimationFrame& getCurrentFrame() const;         // Restituisce il frame attualmente attivo
    void update(int deltaTime);                            // Aggiorna l'animazione in base al tempo passato
    void reset();                                          // Riporta l'animazione al primo frame
    const std::string& getName() const;                    // Restituisce il nome dell'animazione

    bool isFinished() const;                               // Ritorna true se l'animazione non in loop ha raggiunto l'ultimo frame
    void setLoop(bool value);                              // Abilita o disabilita il loop dell'animazione
    bool getLoop() const;                                  // Restituisce lo stato attuale del loop
    void setFrameIndex(int index);                         // Imposta manualmente il frame corrente
    int getFrameIndex() const;                             // Restituisce l'indice del frame corrente

    void saveToFile(const std::string& path) const;        // Salva l'animazione su file
    bool loadFromFile(const std::string& path);            // Carica l'animazione da file

private:
    std::string name;                      // Nome identificativo dell'animazione
    std::vector<AnimationFrame> frames;   // Lista dei frame che compongono l'animazione
    int currentFrameIndex = 0;            // Indice del frame attuale
    int timer = 0;                        // Timer per gestire il tempo del frame corrente
    bool loop = true;                     // Flag per determinare se l'animazione va in loop
};