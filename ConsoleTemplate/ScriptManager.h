#pragma once
#include "GameObjectManager.h"
#include <lua.hpp>
#include <string>
#include <unordered_map>
#include <memory>
#include "NPC.h"
#include "AnimationManager.h"
#include "Entity.h"
#include "SoundManager.h"
//#include "NodeGraph.h"

// Forward declaration :
class Node;
class NodeGraph; // Forward declare NodeGraph if ScriptManager interacts with it in the header


struct ScriptVariable {
    std::string name;
    std::string type;          // Esempio: "number", "string"
    std::string defaultValue;
};

struct ScriptFunction {
    std::string name;
    std::vector<std::string> parameters;
    std::vector<std::shared_ptr<class Node>> bodyNodes; // generati da NodeGraph
};


class NPC;
class Entity;

extern std::unordered_map<std::string, NPC*> npcMap;

class ScriptManager {
public:
    ScriptManager(const std::string& scriptsFolder = "scripts");
    ~ScriptManager();

    void loadScriptsFromFolder(const std::string& folder);
    void bindScriptToEntity(const std::string& entityId, const std::string& scriptName);
    void bindScriptToEntity(Entity* entity, const std::string& scriptName);

    void bindNpc(lua_State* L);
    void bindLuaNPCMemory(lua_State* L);
    void bindSoundAPI();

    // Eventi base
    void onUpdate(const std::string& entityId);
    void onInteract(const std::string& entityId);
    void onClick(const std::string& entityId);
    void onEnterZone(const std::string& entityId, const std::string& zone);
    void onExitZone(const std::string& entityId, const std::string& zone);
    void onDeath(const std::string& entityId);
    void onCollide(const std::string& entityA, const std::string& entityB);

    //void call(const std::string& funcName, NPC& npc);
    void callScriptFunction(const std::string& entityId, const std::string& func);
    void registerFunction(const std::string& luaName, lua_CFunction func);
    void reloadScripts();
    

    // Azioni scriptabili
    void moveEntity(const std::string& entityId, int dx, int dy);
    void say(const std::string& entityId, const std::string& message);
    void spawnEntity(const std::string& type, int x, int y);
    void openObject(const std::string& objectId);
    void closeObject(const std::string& objectId);
    void playSound(const std::string& soundName);
    void addGameObject(GameObject* obj);
    void loadZoneScripts(const std::string& folder);
    void callOnEnterZoneBehavior(int tileID, const std::string& playerName);
    void setSoundManager(SoundManager* manager);

    // Editor & animazioni
    void updateEditorBindings();
    void drawEntityProperties(Entity* entity); // per mostrare gli attributi in editor


    //tabella eventi
    std::vector<std::string> getAvailableEventsForType(const std::string& type) const;

    // Variabili & Funzioni custom
    void addVariable(const ScriptVariable& variable);
    void addFunction(const ScriptFunction& function);

    const std::unordered_map<std::string, ScriptVariable>& getVariables() const;
    const std::unordered_map<std::string, ScriptFunction>& getFunctions() const;



private:
    SoundManager* soundManager = nullptr; // << aggiungi questo

    std::unordered_map<std::string, Entity*> allEntities; // da inserire in private
    GameObjectManager objectManager;
    lua_State* L;
    std::unordered_map<std::string, std::string> entityScripts;
    std::string scriptsDirectory;

    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> npcMemory;
    std::unordered_map<int, int> zoneRefs; // tileID → Lua table reference
    std::unordered_map<std::string, std::vector<std::string>> availableEvents;

    std::unordered_map<std::string, ScriptVariable> userVariables;
    std::unordered_map<std::string, ScriptFunction> userFunctions;

    
    //void bindLuaNPCMemory(lua_State* L);
};
