#include "NodeLibrary.h"
#include <imgui.h>
#include <iostream>

bool NodeLibrary::initialized = false;
std::vector<NodeTemplate> NodeLibrary::nodes;

const std::vector<NodeTemplate>& NodeLibrary::getNodes() {
    if (!initialized) init();


    return nodes;
}

std::vector<std::string> NodeLibrary::getCategories() {
    if (!initialized) init();
    std::unordered_map<std::string, bool> seen;
    std::vector<std::string> out;
    for (const auto& n : nodes) {
        if (!seen[n.category]) {
            out.push_back(n.category);
            seen[n.category] = true;
        }
    }
    return out;
}

std::vector<NodeTemplate> NodeLibrary::getNodesInCategory(const std::string& cat) {
    if (!initialized) init();
    std::vector<NodeTemplate> result;
    for (const auto& n : nodes) {
        if (n.category == cat) result.push_back(n);
    }
    return result;
}

// 🔧 Registra nodi base qui
void NodeLibrary::init() {
    nodes.push_back({
        "Evento onClick", "Event", "Event", IM_COL32(80, 160, 255, 255),
        [](Node& n) {
            n.title = "onClick";
            n.pins.push_back({ 0, PinKind::Out, {n.size.x - 10, n.size.y / 2} });
        }
        });

    nodes.push_back({
        "PlaySound", "PlaySound", "Function", IM_COL32(180, 180, 180, 255),
        [](Node& n) {
            n.title = "sound.wav";
            n.parameters["sound"] = "sound.wav";
            n.pins.push_back({ 0, PinKind::In, {10, n.size.y / 2} });
            n.pins.push_back({ 1, PinKind::Out, {n.size.x - 10, n.size.y / 2} });
        }
        });

    nodes.push_back({
        "SetTile", "SetTile", "Function", IM_COL32(180, 180, 180, 255),
        [](Node& n) {
            n.title = "1";
            n.parameters["tileID"] = "1";
            n.pins.push_back({ 0, PinKind::In, {10, n.size.y / 2} });
            n.pins.push_back({ 1, PinKind::Out, {n.size.x - 10, n.size.y / 2} });
        }
        });

    nodes.push_back({
        "If/Else", "IfElse", "Condition", IM_COL32(255, 150, 200, 255),
        [](Node& n) {
            n.title = "If";
            n.parameters["condition"] = "self.isAlive";
            n.pins.push_back({ 0, PinKind::In, {10, n.size.y / 3} });
            n.pins.push_back({ 1, PinKind::Out, {n.size.x - 10, n.size.y / 3} });
            n.pins.push_back({ 2, PinKind::Out, {n.size.x - 10, 2 * n.size.y / 3} });
        }
        });

    // Aggiunta dinamica di nodi per variabili utente
    auto& vars = ScriptManagerInstance::get()->getVariables();
    for (const auto& [name, var] : vars) {
        nodes.push_back({
            "Set " + name,           // Nome visibile nel menu
            "SetVariable",           // Tipo tecnico del nodo
            "Variable",              // Categoria logica (verde)
            IM_COL32(100, 255, 100, 255),
            [=](Node& n) {
                n.parameters["varName"] = name;
                n.parameters["value"] = var.defaultValue;
                n.pins.push_back({ 0, PinKind::In, {10, n.size.y / 2} });
                n.pins.push_back({ 1, PinKind::Out, {n.size.x - 10, n.size.y / 2} });
            }
            });

        nodes.push_back({
            "Get " + name,
            "GetVariable",
            "Variable",
            IM_COL32(100, 255, 100, 255),
            [=](Node& n) {
                n.parameters["varName"] = name;
                n.pins.push_back({ 0, PinKind::Out, {n.size.x - 10, n.size.y / 2} });
            }
            });
    }

    // Aggiunta dinamica di nodi per funzioni utente
    for (const auto& [name, func] : ScriptManagerInstance::get()->getFunctions()) {
        nodes.push_back({
            "Call " + name,
            "CallFunction",
            "Function",
            IM_COL32(180, 180, 180, 255),
            [=](Node& n) {
                n.parameters["funcName"] = name;
                n.pins.push_back({ 0, PinKind::In, {10, n.size.y / 2} });
                n.pins.push_back({ 1, PinKind::Out, {n.size.x - 10, n.size.y / 2} });
            }
            });
    }

    std::vector<std::string> builtinFunctions = {
    "moveEntity", "say", "spawnEntity", "playSound", "openObject", "closeObject"
    };

    for (const auto& fname : builtinFunctions) {
        nodes.push_back({
            "Call " + fname,
            "CallFunction",
            "SystemFunction",
            IM_COL32(200, 200, 180, 255),
            [=](Node& n) {
                n.parameters["funcName"] = fname;
                n.pins.push_back({ 0, PinKind::In, {10, n.size.y / 2} });
                n.pins.push_back({ 1, PinKind::Out, {n.size.x - 10, n.size.y / 2} });
            }
            });
    }

    NodeTemplate animState;
    animState.name = "AnimState";
    animState.type = "AnimState";
    animState.category = "Animation";
    animState.color = IM_COL32(150, 255, 150, 255);  // Verde chiaro

    animState.onCreate = [](Node& n) {
        n.title = "AnimState";
        n.size = ImVec2(140, 64);
        n.type = "AnimState";
        n.category = "Animation";
        n.color = IM_COL32(150, 255, 150, 255);
        n.pins.clear();
        n.pins.push_back({ 0, PinKind::Out, { n.size.x - 10, n.size.y / 2 } });

        // Parametri obbligatori inizializzati con valori di default
        n.parameters["stateName"] = "idle";
        n.parameters["texturePath"] = "Idle.png";
        n.parameters["frameWidth"] = "96";
        n.parameters["frameHeight"] = "128";
        n.parameters["rows"] = "4";
        n.parameters["cols"] = "3";
        n.parameters["frameTime"] = "0.1";
        n.parameters["loop"] = "true";
        n.parameters["directional"] = "false";

        std::cout << "[DEBUG] Nodo AnimState creato\n";
        };


    nodes.push_back(animState);



    


    initialized = true;
}


void NodeLibrary::refresh() {
    if (!initialized) return;

    // Rimuovi tutti i nodi dinamici esistenti
    nodes.erase(std::remove_if(nodes.begin(), nodes.end(),
        [](const NodeTemplate& n) {
            return n.type == "SetVariable" || n.type == "GetVariable" || n.type == "CallFunction";
        }), nodes.end());

    // Ottieni il manager attivo
    ScriptManager* mgr = ScriptManagerInstance::get();
    if (!mgr) return;

    // Ricrea nodi variabili
    for (const auto& [name, var] : mgr->getVariables()) {
        nodes.push_back({
            "Set " + name,
            "SetVariable",
            "Variable",
            IM_COL32(100, 255, 100, 255),
            [=](Node& n) {
                n.parameters["varName"] = name;
                n.parameters["value"] = var.defaultValue;
                n.pins.push_back({ 0, PinKind::In, {10, n.size.y / 2} });
                n.pins.push_back({ 1, PinKind::Out, {n.size.x - 10, n.size.y / 2} });
            }
            });
        nodes.push_back({
            "Get " + name,
            "GetVariable",
            "Variable",
            IM_COL32(100, 255, 100, 255),
            [=](Node& n) {
                n.parameters["varName"] = name;
                n.pins.push_back({ 0, PinKind::Out, {n.size.x - 10, n.size.y / 2} });
            }
            });
    }

    // Ricrea nodi funzione
    for (const auto& [name, func] : mgr->getFunctions()) {
        nodes.push_back({
            "Call " + name,
            "CallFunction",
            "Function",
            IM_COL32(180, 180, 180, 255),
            [=](Node& n) {
                n.parameters["funcName"] = name;
                n.pins.push_back({ 0, PinKind::In, {10, n.size.y / 2} });
                n.pins.push_back({ 1, PinKind::Out, {n.size.x - 10, n.size.y / 2} });
            }
            });
    }

    std::cout << "[NodeLibrary] Nodi dinamici aggiornati!\n";
}