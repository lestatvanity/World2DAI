// EditorPlayerPanel.cpp
#include "EditorPlayerPanel.h"
#include "AnimationBlueprintintUtils.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <imgui.h>

EditorPlayerPanel::EditorPlayerPanel(AnimationManager& animMgr, AnimationComponent& previewComp)
    : animationManager(animMgr), previewComponent(previewComp) {}

void EditorPlayerPanel::render(const std::vector<Node>& nodes) {
    ImGui::Begin("Editor Player");

    ImGui::Text("Animazione assegnata al player:");
    static int selectedIndex = 0;

    std::vector<std::string> animStates;
    for (const auto& node : nodes) {
        if (node.type == "AnimState") {
            animStates.push_back(node.parameters.at("stateName"));

            // Salva il blueprint per uso futuro
            if (loadedBlueprints.find(node.parameters.at("stateName")) == loadedBlueprints.end()) {
                PlayerAnimationBlueprint bp = buildAnimationBlueprintFromNode(node);
                loadedBlueprints[bp.stateName] = bp;
                animationManager.loadFromBlueprint(bp);
            }
        }
    }

    if (!animStates.empty()) {
        std::vector<const char*> items;
        for (const auto& s : animStates) items.push_back(s.c_str());

        if (ImGui::Combo("Animazione", &selectedIndex, items.data(), static_cast<int>(items.size()))) {
            selectedStateName = animStates[selectedIndex];
            std::cout << "[EditorPlayerPanel] Player userà: " << selectedStateName << "\n";

            if (animationManager.getAll().count(selectedStateName)) {
                previewComponent.addAnimation(selectedStateName, animationManager.get(selectedStateName));
                previewComponent.play(selectedStateName);

                // Carica texture associata
                const auto& bp = loadedBlueprints[selectedStateName];
                if (previewTexture.loadFromFile(bp.texturePath)) {
                    previewSprite.setTexture(previewTexture);
                    previewSprite.setTextureRect({ 0, 0, bp.frameWidth, bp.frameHeight });
                    previewSprite.setScale(1.5f, 1.5f); // opzionale
                }
                else {
                    std::cerr << "[EditorPlayerPanel] Impossibile caricare texture: " << bp.texturePath << "\n";
                }
            }
        }

        // Disegna anteprima sprite
        if (loadedBlueprints.count(selectedStateName)) {
            const auto& bp = loadedBlueprints[selectedStateName];
            const AnimationFrame& frame = previewComponent.getCurrentFrame();
            previewSprite.setTextureRect({ frame.textureX, frame.textureY, bp.frameWidth, bp.frameHeight });

            ImVec2 pos = ImGui::GetCursorScreenPos();
            sf::RenderWindow* window = static_cast<sf::RenderWindow*>(ImGui::GetMainViewport()->PlatformHandleRaw);
            if (window) {
                previewSprite.setPosition(pos.x, pos.y);
                window->draw(previewSprite);
            }

            ImGui::Dummy(ImVec2(bp.frameWidth * 1.5f, bp.frameHeight * 1.5f));
        }
    }
    else {
        ImGui::Text("Nessun AnimState presente.");
    }

    ImGui::End();
}


void EditorPlayerPanel::save(const std::string& path) const {
    nlohmann::json j;
    j["playerAnimState"] = selectedStateName;

    std::ofstream out(path);
    if (out.is_open()) {
        out << j.dump(4);
        std::cout << "[EditorPlayerPanel] Salvato in: " << path << "\n";
    }
}

void EditorPlayerPanel::load(const std::string& path) {
    std::ifstream in(path);
    if (!in.is_open()) return;

    nlohmann::json j;
    in >> j;

    if (j.contains("playerAnimState")) {
        selectedStateName = j["playerAnimState"].get<std::string>();

        if (animationManager.getAll().count(selectedStateName)) {
            previewComponent.addAnimation(selectedStateName, animationManager.get(selectedStateName));
            previewComponent.play(selectedStateName);
        }
    }
}
