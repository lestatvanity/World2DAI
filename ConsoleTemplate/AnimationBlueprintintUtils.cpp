#include "AnimationBlueprintintUtils.h"
#include "NodeGraph.h"
#include <stdexcept>
#include <iostream>


bool validateAnimStateNode(const Node& node, std::vector<std::string>& missing) {
    static const std::vector<std::string> required = {
        "stateName", "texturePath", "frameWidth", "frameHeight",
        "rows", "cols", "frameTime", "loop", "directional"
    };

    missing.clear();
    for (const auto& key : required) {
        if (node.parameters.find(key) == node.parameters.end())
            missing.push_back(key);
    }
    return missing.empty();
}


PlayerAnimationBlueprint buildAnimationBlueprintFromNode(const Node& node) {
    PlayerAnimationBlueprint bp;

    static const std::vector<std::string> keys = {
        "stateName", "texturePath", "frameWidth", "frameHeight",
        "rows", "cols", "frameTime", "loop", "directional"
    };

    for (const std::string& key : keys) {
        if (node.parameters.find(key) == node.parameters.end()) {
            std::cerr << "[AnimState] Parametro mancante: " << key << "\n";
            throw std::runtime_error("Nodo AnimState non valido");
        }
    }

    try {
        bp.stateName = node.parameters.at("stateName");
        bp.texturePath = node.parameters.at("texturePath");
        bp.frameWidth = std::stoi(node.parameters.at("frameWidth"));
        bp.frameHeight = std::stoi(node.parameters.at("frameHeight"));
        bp.rowCount = std::stoi(node.parameters.at("rows"));
        bp.columnCount = std::stoi(node.parameters.at("cols"));
        bp.frameTime = std::stof(node.parameters.at("frameTime"));
        bp.loop = (node.parameters.at("loop") == "true");
        bp.directional = (node.parameters.at("directional") == "true");
    }
    catch (const std::exception& e) {
        std::cerr << "[AnimState] Errore parsing parametro: " << e.what() << "\n";
        throw;
    }

    return bp;
}

