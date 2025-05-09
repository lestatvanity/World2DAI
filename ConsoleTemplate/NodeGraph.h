// NodeGraph.hpp  ───────────────────────────────────────────────
#pragma once
#include "ScriptContext.h"
#include "ScriptManager.h"
#include "NodeLibrary.h"
#include "ScriptManagerInstance.h"
#include "AnimationComponent.h"
#include <SFML/Graphics.hpp>
#include <imgui.h>
#include <unordered_map>
#include <vector>
#include <unordered_set>   // aggiunto!
#include <queue>
#include <optional>





class ScriptManager;           // ❗️Forward declaration


enum class PinKind { In, Out };

struct Pin {
    int         id;
    PinKind     kind;
    ImVec2      offset;   // offset locale all’interno del nodo
};

struct Node {
    int                 id;
    ImVec2              pos{ 0,0 };
    ImVec2              size{ 140, 64 };
    std::string         title = "Node";
    std::string         type = "Generic";     // Tipo tecnico ("PlaySound", "SetTile", ecc.)
    std::string         category = "Generic"; // Logica: "Event", "Variable", "Function", "Condition"
    ImU32               color = IM_COL32(255, 255, 255, 255); // Colore nodo
    std::vector<Pin>    pins;
    std::unordered_map<std::string, std::string> parameters;
    bool hasError = false;
    std::vector<std::string> missingParams;

};


struct Link {
    int fromNode, fromPin;
    int toNode, toPin;
};

struct CommentBox {
    ImVec2 pos;
    ImVec2 size;
    std::string text;
    ImU32 color = IM_COL32(100, 100, 255, 60);
};

extern std::vector<CommentBox> commentBoxes;


class NodeGraph {
public:

    //csotruttore
    NodeGraph();

    // chiamare nei tre momenti classici
    void handleEvent(const sf::Event&, const sf::RenderWindow&);
    void update(float dt);
    void draw();
    void generateLua(const std::string& path, ScriptManager& scriptManager);
    void renderProperties();
    void autoLayout();


    // save e load json
    void save(const std::string& path) const;
    void load(const std::string& path);

    // dati pubblici (provvisorio)
    std::vector<Node> nodes;
    std::vector<Link> links;
    // Avvia la simulazione del flow
    void startFlow();

    bool isLinkValid(const Node& fromNode, const Pin& fromPin, const Node& toNode, const Pin& toPin) const;
    void handlePinClick(const Node& N, const Pin& pin);



private:

    void triggerAutoGenerate() {
        if (ScriptManagerInstance::get()) {
            generateLua("scripts/generated_script.lua", *ScriptManagerInstance::get());
        }
    }


    // interazione
    int  dragNodeId = -1;
    ImVec2 dragOffset = { 0,0 };

    bool draggingLink = false;
    int  linkFromNode = -1, linkFromPin = -1;

    //ID dei nodi
    int nextNodeId;

    // panning / zoom
    ImVec2  viewOffset = { 0,0 };
    float   zoom = 1.f;

    int selectedNodeId = -1; // ID del nodo selezionato


    // helpers
    // prima
// ImVec2 NodeGraph::worldToScreen(ImVec2 p) const { return (p - viewOffset) * zoom; }
// ImVec2 NodeGraph::screenToWorld(ImVec2 p) const { return p / zoom + viewOffset; }

// dopo
    ImVec2 worldToScreen(const ImVec2& p) const;

    ImVec2 screenToWorld(const ImVec2& p) const;

    int    pinAt(const ImVec2& mouse);
    std::optional<std::pair<int, int>> pinAtDetailed(const ImVec2& mouse);

    bool selectingArea = false;
    ImVec2 selectionStart = { 0,0 };
    ImVec2 selectionEnd = { 0,0 };
    std::unordered_set<int> selectedNodes;

    // --- FLOW LIVE ---
    bool flowActive = false;
    float flowTimer = 0.0f;
    float flowDelay = 0.5f;
    std::queue<int> flowQueque;
    std::unordered_set<int> activeNodes;


    float activeTimer = 0.0f;
    //std::unordered_set<int> selectedNodes;  // <<<<<< QUI!!

    int dragCommentId = -1;       // ID del commento trascinato
    ImVec2 dragCommentOffset = {}; // offset tra mouse e pos iniziale

    bool lastLinkWasInvalid = false;

    ScriptContext ctx;
    ScriptManager scriptManager;
    AnimationManager animationManager;


    int selectedCommentId = -1;

    // Centro canvas
    ImVec2 canvasCenter = {};

    AnimationComponent previewComponent;
    sf::Texture previewTexture;
    sf::Sprite previewSprite;

    std::string selectedPlayerAnimState = "";

    float errorBlinkTimer = 0.0f;

};
