#pragma once
#include <memory>
#include "Animation.h"
#include "PlayerAnimationBlueprint.h"

class AnimationFactory {
public:
    static std::shared_ptr<Animation> createFromBlueprint(const PlayerAnimationBlueprint& blueprint) {
        std::shared_ptr<Animation> anim = std::make_shared<Animation>(blueprint.stateName);

        int frameCount = blueprint.rowCount * blueprint.columnCount;
        for (int row = 0; row < blueprint.rowCount; ++row) {
            for (int col = 0; col < blueprint.columnCount; ++col) {
                int x = col * blueprint.frameWidth;
                int y = row * blueprint.frameHeight;
                anim->addFrame(x, y, static_cast<int>(blueprint.frameTime * 1000.0f));
            }
        }

        anim->setLoop(blueprint.loop);
        return anim;
    }
};
