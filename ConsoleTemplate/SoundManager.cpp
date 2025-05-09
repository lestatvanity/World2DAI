#include "SoundManager.h"
#include <iostream>

bool SoundManager::loadSound(const std::string& name, const std::string& filepath) {
    auto buffer = std::make_unique<sf::SoundBuffer>();
    if (!buffer->loadFromFile(filepath)) {
        std::cerr << "[SoundManager] Impossibile caricare suono da: " << filepath << "\n";
        return false;
    }
    soundBuffers[name] = std::move(buffer);
    sf::Sound sound;
    sound.setBuffer(*soundBuffers[name]);
    activeSounds[name] = sound;
    return true;
}

void SoundManager::playSound(const std::string& name, bool loop, float volume) {
    if (!hasSound(name)) {
        std::cerr << "[SoundManager] Suono '" << name << "' non caricato!\n";
        return;
    }

    sf::Sound& sound = activeSounds[name];
    sound.setLoop(loop);
    sound.setVolume(volume * 100.f); // da 0.0 a 1.0 → 0 a 100
    sound.play();
}

void SoundManager::stopSound(const std::string& name) {
    if (hasSound(name)) {
        activeSounds[name].stop();
    }
}

void SoundManager::stopAll() {
    for (auto& [name, sound] : activeSounds) {
        sound.stop();
    }
}

bool SoundManager::hasSound(const std::string& name) const {
    return soundBuffers.find(name) != soundBuffers.end();
}
