#pragma once
#include "imgui_includes.h"
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include <unordered_map>

struct FrameData {
    int x, y, width, height;
};

struct AnimationData {
    std::string name;
    float frameTime = 0.1f;
    std::vector<FrameData> frames;
};

class AnimationPanel {
public:
    AnimationPanel();

    void update();
    void render();
    void loadSpritesheet(const std::string& path);
    void saveToJson(const std::string& path);
    void loadFromJson(const std::string& path);

private:
    sf::Texture spritesheetTexture;
    sf::Sprite previewSprite;
    bool showGrid = false;

    std::vector<AnimationData> animations;
    int selectedAnimationIndex = -1;

    int currentFrameX = 0;
    int currentFrameY = 0;
    int frameWidth = 32;
    int frameHeight = 32;
    float currentFrameTime = 0.1f;
    char animNameBuffer[64] = "";

    void drawPreview();
    void drawAnimationEditor();
};
