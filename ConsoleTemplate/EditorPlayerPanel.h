// EditorPlayerPanel.h
#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include "AnimationComponent.h"
#include "AnimationManager.h"
#include "NodeGraph.h"

class EditorPlayerPanel {
public:
    EditorPlayerPanel(AnimationManager& animMgr, AnimationComponent& previewComp);

    void render(const std::vector<Node>& nodes); // mostra il pannello
    void save(const std::string& path) const;    // salva su json
    void load(const std::string& path);          // carica da json

private:
    std::string selectedStateName = "";
    AnimationManager& animationManager;
    AnimationComponent& previewComponent;
    sf::Texture previewTexture;
    sf::Sprite previewSprite;
    std::unordered_map<std::string, PlayerAnimationBlueprint> loadedBlueprints;


};