#include "AnimationComponent.h"

// Aggiunge un'animazione alla mappa con il nome fornito
void AnimationComponent::addAnimation(const std::string& name, const Animation& anim) {
    animations[name] = anim; // Inserisce o sovrascrive l'animazione nella mappa
}

// Cambia l'animazione attiva se diversa da quella corrente
void AnimationComponent::play(const std::string& name) {
    if (name != current && animations.count(name)) { // Solo se diversa dalla corrente e esiste nella mappa
        current = name;                              // Imposta la nuova animazione come attiva
        animations[current].reset();                 // Resetta il ciclo della nuova animazione
    }
}

// Aggiorna l'animazione attiva in base al tempo
void AnimationComponent::update(int deltaTime) {
    if (!current.empty()) {                          // Controlla se un'animazione è attiva
        animations[current].update(deltaTime);       // Propaga l'update alla relativa animazione
    }
}

// Restituisce il frame attuale dell'animazione in corso
const AnimationFrame& AnimationComponent::getCurrentFrame() const {
    return animations.at(current).getCurrentFrame(); // Accesso sicuro alla mappa con .at()
}

// Restituisce il nome dell'animazione in esecuzione
std::string AnimationComponent::getCurrentAnimationName() const {
    return current; // Ritorna il nome della chiave corrente
}


void AnimationComponent::addAnimation(const std::string& name, std::shared_ptr<Animation> anim) {
    if (anim) {
        animations[name] = *anim; // copia il contenuto nella mappa esistente
    }
}
