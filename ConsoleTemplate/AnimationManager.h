#pragma once
#include "AnimationComponent.h"
#include "PlayerAnimationBlueprint.h"
#include "AnimationFactory.h"
#include <unordered_map>
#include <string>
#include <memory>

class AnimationManager {
public:
    void add(const std::string& name, const Animation& anim);
    const Animation& get(const std::string& name) const;

    void loadFromFile(const std::string& path);
    void saveToFile(const std::string& path);

    void updateAll(float dt);
    void registerComponent(const std::string& id, std::shared_ptr<AnimationComponent> comp);

    void loadFromBlueprint(const PlayerAnimationBlueprint& blueprint); // firma corretta

    const std::unordered_map<std::string, std::shared_ptr<Animation>>& getAll() const {
        return animationsByState;
    }


private:
    std::unordered_map<std::string, Animation> animations;
    std::unordered_map<std::string, std::shared_ptr<AnimationComponent>> components;

    // opzionale: solo se vuoi animazioni collegate per stato
    std::unordered_map<std::string, std::shared_ptr<Animation>> animationsByState;

};
