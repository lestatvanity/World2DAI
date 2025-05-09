// NodeLibrary.h
#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <imgui.h>  // <-- necessario per ImU32
#include "NodeGraph.h" // Per usare struct Node

struct NodeTemplate {
    std::string name;
    std::string type;
    std::string category;
    ImU32 color;
    std::function<void(Node&)> onCreate;
};

class NodeLibrary {
public:
    static const std::vector<NodeTemplate>& getNodes();
    static std::vector<std::string> getCategories();
    static std::vector<NodeTemplate> getNodesInCategory(const std::string& cat);
    static void refresh();

private:
    static void init();
    static bool initialized;
    static std::vector<NodeTemplate> nodes;
    ImU32 color;

};
