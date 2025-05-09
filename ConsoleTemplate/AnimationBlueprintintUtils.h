#pragma once
#include "PlayerAnimationBlueprint.h"
#include "NodeGraph.h"  // QUESTA � FONDAMENTALE per vedere la struct Node

PlayerAnimationBlueprint buildAnimationBlueprintFromNode(const Node& node);
bool validateAnimStateNode(const Node& node, std::vector<std::string>& missing);

