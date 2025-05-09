#include "imgui_includes.h"
#include "AnimationPanel.h"
#include <imgui-SFML.h>
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>

using json = nlohmann::json;

AnimationPanel::AnimationPanel() {
    memset(animNameBuffer, 0, sizeof(animNameBuffer));
}

void AnimationPanel::update() {
    drawAnimationEditor();
}

void AnimationPanel::render() {
    drawPreview();
}

void AnimationPanel::loadSpritesheet(const std::string& path) {
    if (!spritesheetTexture.loadFromFile(path)) {
        std::cerr << "[AnimationPanel] Errore caricamento spritesheet: " << path << "\n";
        return;
    }
    previewSprite.setTexture(spritesheetTexture);
}

void AnimationPanel::drawPreview() {
    ImGui::Begin("Anteprima Spritesheet");

    sf::Vector2u size = spritesheetTexture.getSize();
    ImGui::Text("Dimensioni: %u x %u", size.x, size.y);

    ImTextureID texID = (ImTextureID)&spritesheetTexture;
    ImGui::Image(texID, ImVec2((float)size.x, (float)size.y));

    ImGui::End();
}

void AnimationPanel::drawAnimationEditor() {
    ImGui::Begin("Gestione Animazioni");

    ImGui::InputText("Nome animazione", animNameBuffer, sizeof(animNameBuffer));
    ImGui::InputInt("Frame X", &currentFrameX);
    ImGui::InputInt("Frame Y", &currentFrameY);
    ImGui::InputInt("Larghezza", &frameWidth);
    ImGui::InputInt("Altezza", &frameHeight);
    ImGui::InputFloat("Durata frame", &currentFrameTime);

    if (ImGui::Button("Aggiungi Frame")) {
        if (selectedAnimationIndex >= 0) {
            FrameData frame{ currentFrameX, currentFrameY, frameWidth, frameHeight };
            animations[selectedAnimationIndex].frames.push_back(frame);
        }
    }

    if (ImGui::Button("Crea nuova animazione")) {
        AnimationData anim;
        anim.name = animNameBuffer;
        anim.frameTime = currentFrameTime;
        animations.push_back(anim);
        selectedAnimationIndex = (int)animations.size() - 1;
        memset(animNameBuffer, 0, sizeof(animNameBuffer));
    }

    if (!animations.empty()) {
        ImGui::Separator();
        ImGui::Text("Animazioni esistenti:");
        for (size_t i = 0; i < animations.size(); ++i) {
            if (ImGui::Selectable(animations[i].name.c_str(), selectedAnimationIndex == (int)i)) {
                selectedAnimationIndex = (int)i;
            }
        }

        if (selectedAnimationIndex >= 0) {
            ImGui::Text("Frames: %zu", animations[selectedAnimationIndex].frames.size());
        }
    }

    if (ImGui::Button("Salva JSON")) {
        saveToJson("assets/animazioni.json");
    }

    if (ImGui::Button("Carica JSON")) {
        loadFromJson("assets/animazioni.json");
    }

    ImGui::End();
}

void AnimationPanel::saveToJson(const std::string& path) {
    json j;
    for (const auto& anim : animations) {
        json animJson;
        animJson["name"] = anim.name;
        animJson["frameTime"] = anim.frameTime;
        for (const auto& frame : anim.frames) {
            animJson["frames"].push_back({ {"x", frame.x}, {"y", frame.y}, {"width", frame.width}, {"height", frame.height} });
        }
        j["animations"].push_back(animJson);
    }

    std::ofstream out(path);
    out << j.dump(4);
    std::cout << "[AnimationPanel] Animazioni salvate in: " << path << "\n";
}

void AnimationPanel::loadFromJson(const std::string& path) {
    std::ifstream in(path);
    if (!in.is_open()) return;

    json j;
    in >> j;
    animations.clear();

    for (const auto& animJson : j["animations"]) {
        AnimationData anim;
        anim.name = animJson["name"];
        anim.frameTime = animJson["frameTime"];

        for (const auto& frameJson : animJson["frames"]) {
            FrameData frame;
            frame.x = frameJson["x"];
            frame.y = frameJson["y"];
            frame.width = frameJson["width"];
            frame.height = frameJson["height"];
            anim.frames.push_back(frame);
        }

        animations.push_back(anim);
    }

    std::cout << "[AnimationPanel] Animazioni caricate da: " << path << "\n";
}
