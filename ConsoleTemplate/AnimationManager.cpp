#include "AnimationManager.h"
#include "AnimationFactory.h"
#include <iostream>

void AnimationManager::add(const std::string& name, const Animation& anim) {
    animations[name] = anim;
    std::cout << "[AnimationManager] Aggiunta animazione: " << name << "\n";
}

const Animation& AnimationManager::get(const std::string& name) const {
    auto it = animations.find(name);
    if (it == animations.end()) {
        throw std::runtime_error("[AnimationManager] Animazione '" + name + "' non trovata.");
    }
    return it->second;
}

void AnimationManager::loadFromFile(const std::string& path) {
    // TODO: parsing JSON o file custom
    std::cout << "[AnimationManager] Caricamento da file non ancora implementato: " << path << "\n";
}

void AnimationManager::saveToFile(const std::string& path) {
    // TODO: serializzazione JSON o file custom
    std::cout << "[AnimationManager] Salvataggio su file non ancora implementato: " << path << "\n";
}


void AnimationManager::updateAll(float dt) {
    for (auto& [id, comp] : components) {
        if (comp)
            comp->update(dt);
    }
}

void AnimationManager::loadFromBlueprint(const PlayerAnimationBlueprint& blueprint) {
    auto animation = AnimationFactory::createFromBlueprint(blueprint);
    if (!animation) {
        std::cerr << "[AnimationManager] Errore: animazione nulla per stato " << blueprint.stateName << "\n";
        return;
    }
    animationsByState[blueprint.stateName] = animation;
}
