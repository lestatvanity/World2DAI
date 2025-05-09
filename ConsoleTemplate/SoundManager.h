#pragma once
#include <SFML/Audio.hpp>
#include <unordered_map>
#include <string>
#include <memory>

class SoundManager {
public:
    bool loadSound(const std::string& name, const std::string& filepath);
    void playSound(const std::string& name, bool loop, float volume);
    void stopSound(const std::string& name);
    bool hasSound(const std::string& name) const;
    void stopAll();

private:
    std::unordered_map<std::string, std::unique_ptr<sf::SoundBuffer>> soundBuffers;
    std::unordered_map<std::string, sf::Sound> activeSounds;
};
