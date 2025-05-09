#pragma once
#include "Animation.h"
#include <unordered_map>
#include <string>

// Componente che gestisce un set di animazioni per un'entità
// Può contenere più animazioni identificate per nome, e aggiorna quella attiva
class AnimationComponent {
public:
    void addAnimation(const std::string& name, const Animation& anim); // Aggiunge una nuova animazione con nome identificativo
    void addAnimation(const std::string& name, std::shared_ptr<Animation> anim); // ✔️ nuova overload

    void play(const std::string& name);                                // Attiva l'animazione corrispondente al nome
    void update(int deltaTime);                                        // Aggiorna l'animazione attiva in base al tempo trascorso

    const AnimationFrame& getCurrentFrame() const;                     // Restituisce il frame corrente dell'animazione attiva
    std::string getCurrentAnimationName() const;                       // Restituisce il nome dell'animazione attiva

private:
    std::unordered_map<std::string, Animation> animations; // Mappa delle animazioni, indicizzate per nome
    std::string current;                                   // Nome dell'animazione attualmente in esecuzione
};