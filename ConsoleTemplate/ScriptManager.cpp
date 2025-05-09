#include "ScriptManager.h"
#include "GameObject.h"
#include "NodeGraph.h"
#include <filesystem>
#include <iostream>
#include <fstream>
#include <vector> // se non l’hai già incluso
#include <memory> // per shared_ptr dei nodi




extern std::unordered_map<std::string, GameObject*> objectMap;

std::unordered_map<std::string, NPC*> npcMap; // << QUI

// ...resto del codice ScriptManager.cpp



ScriptManager::ScriptManager(const std::string& scriptsFolder)
    : scriptsDirectory(scriptsFolder) {
    L = luaL_newstate();
    luaL_openlibs(L);
    bindSoundAPI(); // <-- qui
    // Definizione eventi validi per tipi
    availableEvents["tile"] = { "onClick", "onStep" };
    availableEvents["group"] = { "onUpdate", "onActivated" };
    availableEvents["entity"] = { "onInteract", "onUpdate", "onDeath", "onSpawn", "onCollide" };
    availableEvents["area"] = { "onEnterArea", "onExitArea" };
    // Protezione anti-crash Lua
    lua_atpanic(L, [](lua_State* L) -> int {
        std::cerr << "[Lua PANIC] " << lua_tostring(L, -1) << std::endl;
        return 0;
        });

    // Aggiunge la cartella scripts al path di ricerca
    luaL_dostring(L, "package.path = package.path .. ';scripts/?.lua'");

    
    // 💡 Funzioni di sistema base per qualsiasi gioco 2D
    addFunction({ "playSound", {"soundName"}, {} });
    addFunction({ "moveEntity", {"entityId", "dx", "dy"}, {} });
    addFunction({ "say", { std::string("entityId"), std::string("message") }, {} });
    addFunction({ "spawnEntity", {"type", "x", "y"}, {} });
    addFunction({ "openObject", {"objectId"}, {} });
    addFunction({ "closeObject", {"objectId"}, {} });
    addFunction({ "setMemory", {"entityId", "key", "value"}, {} });
    addFunction({ "getMemory", {"entityId", "key"}, {} });


}

ScriptManager::~ScriptManager() {
    lua_close(L);
}


void ScriptManager::loadScriptsFromFolder(const std::string& folder) {
    for (const auto& file : std::filesystem::directory_iterator(folder)) {
        if (file.path().extension() == ".lua") {
            if (luaL_dofile(L, file.path().string().c_str()) != LUA_OK) {
                std::cerr << "Errore script: " << lua_tostring(L, -1) << std::endl;
                lua_pop(L, 1);
            }
        }
    }
}



void ScriptManager::bindNpc(lua_State* L) {
    lua_newtable(L); // metatable

    lua_pushcfunction(L, [](lua_State* L) -> int {
        NPC* npc = *static_cast<NPC**>(luaL_checkudata(L, 1, "NPC"));
        lua_pushstring(L, npc->getID().c_str());
        return 1;
        });
    lua_setfield(L, -2, "getID");

    lua_pushcfunction(L, [](lua_State* L) -> int {
        NPC* npc = *static_cast<NPC**>(luaL_checkudata(L, 1, "NPC"));
        lua_pushinteger(L, npc->getX());
        return 1;
        });
    lua_setfield(L, -2, "getX");

    lua_pushcfunction(L, [](lua_State* L) -> int {
        NPC* npc = *static_cast<NPC**>(luaL_checkudata(L, 1, "NPC"));
        lua_pushinteger(L, npc->getY());
        return 1;
        });
    lua_setfield(L, -2, "getY");

    lua_pushcfunction(L, [](lua_State* L) -> int {
        NPC* npc = *static_cast<NPC**>(luaL_checkudata(L, 1, "NPC"));
        int dx = luaL_checkinteger(L, 2);
        int dy = luaL_checkinteger(L, 3);
        npc->moveBy(dx, dy);
        return 0;
        });
    lua_setfield(L, -2, "moveBy");

    lua_setglobal(L, "NPC"); // table completa

    // Aggiungi binding memoria
    bindLuaNPCMemory(L);

    lua_pop(L, 1);
}



void ScriptManager::callScriptFunction(const std::string& entityId, const std::string& func) {
    if (!entityScripts.count(entityId)) {
        std::cerr << "[Lua] Nessuno script associato all'entità '" << entityId << "'\n";
        return;
    }

    std::string globalName = "entity_" + entityId;
    lua_getglobal(L, globalName.c_str());

    if (!lua_istable(L, -1)) {
        std::cerr << "[Lua] Tabella Lua non trovata per '" << globalName << "'. Tipo: "
            << luaL_typename(L, -1) << "\n";
        lua_pop(L, 1);
        return;
    }

    lua_getfield(L, -1, func.c_str());

    if (!lua_isfunction(L, -1)) {
        std::cerr << "[Lua]  '" << func << "' non è una funzione valida per '" << globalName << "'\n";
        lua_pop(L, 2);  // pop funzione non valida e tabella
        return;
    }

    lua_pushstring(L, entityId.c_str());

    int status = lua_pcall(L, 1, 0, 0);
    if (status != LUA_OK) {
        const char* error = lua_tostring(L, -1);
        std::cerr << "[Lua] Errore eseguendo '" << func << "' per '" << entityId << "': "
            << (error ? error : "Errore sconosciuto") << "\n";
        lua_pop(L, 1);
    }

    lua_pop(L, 1); // rimuove la tabella globale
}

void ScriptManager::onEnterZone(const std::string& entityId, const std::string& zone) {
    lua_getglobal(L, "onEnterZone");
    if (lua_isfunction(L, -1)) {
        lua_pushstring(L, entityId.c_str());
        lua_pushstring(L, zone.c_str()); // Passa il parametro 'zone'
        if (lua_pcall(L, 2, 0, 0) != LUA_OK) {
            std::cerr << "Errore: " << lua_tostring(L, -1) << std::endl;
            lua_pop(L, 1);
        }
    }
    else {
        lua_pop(L, 1);
    }
}


void ScriptManager::onUpdate(const std::string& entityId) { callScriptFunction(entityId, "onUpdate"); }
void ScriptManager::onInteract(const std::string& entityId) { callScriptFunction(entityId, "onInteract"); }
void ScriptManager::onClick(const std::string& entityId) { callScriptFunction(entityId, "onClick"); }

void ScriptManager::onExitZone(const std::string& entityId, const std::string& zone) {
    callScriptFunction(entityId, "onExitZone"); // migliorabile con parametri
}
void ScriptManager::onDeath(const std::string& entityId) { callScriptFunction(entityId, "onDeath"); }

void ScriptManager::moveEntity(const std::string& entityId, int dx, int dy) {
    auto it = npcMap.find(entityId);
    if (it != npcMap.end()) {
        it->second->moveBy(dx, dy);
    }
    else {
        std::cerr << "[Lua] Errore: NPC " << entityId << " non trovato per moveEntity\n";
    }
}

void ScriptManager::say(const std::string& entityId, const std::string& message) {
    auto it = npcMap.find(entityId);
    if (it != npcMap.end()) {
        it->second->say(message);  // Assicurati che NPC abbia say(string)
    }
    else {
        std::cerr << "[Lua] Errore: NPC " << entityId << " non trovato per say\n";
    }
}

void ScriptManager::spawnEntity(const std::string& type, int x, int y) {
    static int spawnId = 0;
    std::string id = "npc_spawn_" + std::to_string(spawnId++);
    NPC* newNpc = new NPC(id, x, y);
    npcMap[id] = newNpc;
    std::cout << "[Lua] Spawno NPC '" << id << "' di tipo '" << type << "' a (" << x << "," << y << ")\n";
    // puoi chiamare: scriptManager.bindScriptToEntity(id, "defaultAI");
    // e aggiungerlo alla mappa del gioco, rendering, ecc.
}

void ScriptManager::openObject(const std::string& objectId) {
    GameObject* obj = objectManager.getObject(objectId);
    if (obj) {
        obj->open();
        std::cout << "[Lua] Oggetto '" << objectId << "' aperto!\n";
    }
    else {
        std::cerr << "[Lua] Errore: Oggetto '" << objectId << "' non trovato!\n";
    }
}

void ScriptManager::closeObject(const std::string& objectId) {
    GameObject* obj = objectManager.getObject(objectId);
    if (obj) {
        obj->close();
        std::cout << "[Lua] Oggetto '" << objectId << "' chiuso!\n";
    }
    else {
        std::cerr << "[Lua] Errore: Oggetto '" << objectId << "' non trovato!\n";
    }
}


void ScriptManager::playSound(const std::string& soundName) {
    if (!soundManager) {
        std::cerr << "[Lua] SoundManager non impostato!\n";
        return;
    }

    bool loop = false;
    float volume = 100.f;

    // Se c'è un secondo argomento ed è una tabella
    if (lua_gettop(L) >= 2 && lua_istable(L, 2)) {
        lua_getfield(L, 2, "loop");
        if (lua_isboolean(L, -1)) {
            loop = lua_toboolean(L, -1);
        }
        lua_pop(L, 1); // rimuovi il valore letto

        lua_getfield(L, 2, "volume");
        if (lua_isnumber(L, -1)) {
            volume = static_cast<float>(lua_tonumber(L, -1));
        }
        lua_pop(L, 1); // rimuovi il valore letto
    }

    soundManager->playSound(soundName, loop, volume);
}



void ScriptManager::registerFunction(const std::string& luaName, lua_CFunction func) {
    lua_register(L, luaName.c_str(), func);
}

void ScriptManager::bindLuaNPCMemory(lua_State* L) {
    lua_pushcfunction(L, [](lua_State* L) -> int {
        NPC* npc = *static_cast<NPC**>(luaL_checkudata(L, 1, "NPC"));
        const char* key = luaL_checkstring(L, 2);
        const char* value = luaL_checkstring(L, 3);
        ScriptManager* manager = static_cast<ScriptManager*>(lua_touserdata(L, lua_upvalueindex(1)));
        manager->npcMemory[npc->getID()][key] = value;
        return 0;
        });
    lua_pushlightuserdata(L, this);
    lua_setupvalue(L, -2, 1);
    lua_setfield(L, -2, "setMemory");

    lua_pushcfunction(L, [](lua_State* L) -> int {
        NPC* npc = *static_cast<NPC**>(luaL_checkudata(L, 1, "NPC"));
        const char* key = luaL_checkstring(L, 2);
        ScriptManager* manager = static_cast<ScriptManager*>(lua_touserdata(L, lua_upvalueindex(1)));
        std::string value = manager->npcMemory[npc->getID()][key];
        lua_pushstring(L, value.c_str());
        return 1;
        });
    lua_pushlightuserdata(L, this);
    lua_setupvalue(L, -2, 1);
    lua_setfield(L, -2, "getMemory");
}

void ScriptManager::reloadScripts() {
    std::cout << "[Lua] Ricarico tutti gli script...\n";
    loadScriptsFromFolder(scriptsDirectory);
}
void ScriptManager::onCollide(const std::string& entityA, const std::string& entityB) {
    lua_getglobal(L, "onCollide");
    if (lua_isfunction(L, -1)) {
        lua_pushstring(L, entityA.c_str());
        lua_pushstring(L, entityB.c_str());
        if (lua_pcall(L, 2, 0, 0) != LUA_OK) {
            std::cerr << "[Lua] Errore in onCollide: " << lua_tostring(L, -1) << std::endl;
            lua_pop(L, 1);
        }
    }
    else {
        lua_pop(L, 1);
    }
}

void ScriptManager::addGameObject(GameObject* obj) {
    objectManager.addObject(obj);
}


void ScriptManager::loadZoneScripts(const std::string& folder) {
    namespace fs = std::filesystem;

    for (const auto& entry : fs::directory_iterator(folder)) {
        if (entry.path().extension() == ".lua") {
            int tileID = std::stoi(entry.path().stem().string()); // es: 3.lua → tileID = 3

            if (luaL_loadfile(L, entry.path().string().c_str()) == LUA_OK &&
                lua_pcall(L, 0, 1, 0) == LUA_OK) {

                // Memorizziamo la tabella in una ref
                int ref = luaL_ref(L, LUA_REGISTRYINDEX);
                zoneRefs[tileID] = ref;

                std::cout << "[Lua] Caricato comportamento per tile ID " << tileID << "\n";
            }
            else {
                std::cerr << "[Lua] Errore in " << entry.path() << ": " << lua_tostring(L, -1) << "\n";
                lua_pop(L, 1);
            }
        }
    }
}

void ScriptManager::callOnEnterZoneBehavior(int tileID, const std::string& playerName) {
    auto it = zoneRefs.find(tileID);
    if (it == zoneRefs.end()) return;

    lua_rawgeti(L, LUA_REGISTRYINDEX, it->second); // la tabella
    lua_getfield(L, -1, "onEnter");

    if (lua_isfunction(L, -1)) {
        lua_pushstring(L, playerName.c_str());
        lua_pushinteger(L, tileID);

        if (lua_pcall(L, 2, 0, 0) != LUA_OK) {
            std::cerr << "[Lua] Errore in onEnter per tile " << tileID << ": " << lua_tostring(L, -1) << "\n";
            lua_pop(L, 1);
        }
    }

    lua_pop(L, 1); // pop la tabella
}


void ScriptManager::bindScriptToEntity(Entity* entity, const std::string& scriptName) {
    const std::string& id = entity->getID();
    std::string path = scriptsDirectory + "/" + scriptName + ".lua";

    if (luaL_loadfile(L, path.c_str()) != LUA_OK) {
        std::cerr << "[Lua] Errore nel loadfile di " << path << ": " << lua_tostring(L, -1) << "\n";
        lua_pop(L, 1);
        return;
    }

    if (lua_pcall(L, 0, 1, 0) != LUA_OK) {
        std::cerr << "[Lua] Errore nell'esecuzione di " << path << ": " << lua_tostring(L, -1) << "\n";
        lua_pop(L, 1);
        return;
    }

    if (!lua_istable(L, -1)) {
        std::cerr << "[Lua] Lo script " << scriptName << " non ha restituito una tabella\n";
        lua_pop(L, 1);
        return;
    }

    lua_pushvalue(L, -1);
    lua_setglobal(L, ("entity_" + id).c_str());
    lua_pop(L, 1);

    allEntities[id] = entity;
    entityScripts[id] = scriptName;

    std::cout << "[Lua] Script '" << scriptName << "' associato a entità '" << id << "'\n";
}

void ScriptManager::bindScriptToEntity(const std::string& entityId, const std::string& scriptName) {
    std::string path = scriptsDirectory + "/" + scriptName + ".lua";

    if (luaL_loadfile(L, path.c_str()) != LUA_OK) {
        std::cerr << "[Lua] Errore nel loadfile di " << path << ": " << lua_tostring(L, -1) << "\n";
        lua_pop(L, 1);
        return;
    }

    if (lua_pcall(L, 0, 1, 0) != LUA_OK) {
        std::cerr << "[Lua] Errore nell'esecuzione di " << path << ": " << lua_tostring(L, -1) << "\n";
        lua_pop(L, 1);
        return;
    }

    if (!lua_istable(L, -1)) {
        std::cerr << "[Lua] Lo script " << scriptName << " NON ha restituito una tabella! Tipo: "
            << luaL_typename(L, -1) << "\n";
        lua_pop(L, 1);
        return;
    }

    lua_pushvalue(L, -1); // duplica la tabella
    lua_setglobal(L, ("entity_" + entityId).c_str()); // salva con nome globale
    lua_pop(L, 1); // rimuove la tabella originale

    entityScripts[entityId] = scriptName;
    std::cout << "[Lua] Script '" << scriptName << "' associato a entità '" << entityId << "'\n";
}


void ScriptManager::updateEditorBindings() {
    for (auto& [id, entity] : allEntities) {
        if (entity) {
            // In futuro qui potremo integrare property inspectors, update visivi, ecc.
            entity->updateVisual(); // funzione virtuale nelle sottoclassi
        }
    }
}


void ScriptManager::drawEntityProperties(Entity* entity) {
    if (!entity) return;

    std::cout << "===== Proprietà di " << entity->getID() << " =====\n";
    std::cout << "Posizione: (" << entity->getX() << ", " << entity->getY() << ")\n";
    std::cout << "Tipo: " << entity->getType() << "\n";
    std::cout << "Animazione corrente: " << entity->getCurrentAnimation() << "\n";
}

void ScriptManager::setSoundManager(SoundManager* manager) {
    soundManager = manager;
}
void ScriptManager::bindSoundAPI() {
    if (!soundManager) {
        std::cerr << "[ScriptManager] soundManager non inizializzato!\n";
        return;
    }

    lua_newtable(L);

    // Sound.play(name, options)
    lua_pushlightuserdata(L, soundManager);
    lua_pushcclosure(L, [](lua_State* L) -> int {
        SoundManager* mgr = static_cast<SoundManager*>(lua_touserdata(L, lua_upvalueindex(1)));

        const char* name = luaL_checkstring(L, 1);
        bool loop = false;
        float volume = 1.0f;

        if (lua_istable(L, 2)) {
            lua_getfield(L, 2, "loop");
            if (lua_isboolean(L, -1)) loop = lua_toboolean(L, -1);
            lua_pop(L, 1);

            lua_getfield(L, 2, "volume");
            if (lua_isnumber(L, -1)) volume = static_cast<float>(lua_tonumber(L, -1));
            lua_pop(L, 1);
        }

        mgr->playSound(name, loop, volume);
        return 0;
        }, 1);
    lua_setfield(L, -2, "play");

    // Sound.stop(name)
    lua_pushlightuserdata(L, soundManager);
    lua_pushcclosure(L, [](lua_State* L) -> int {
        SoundManager* mgr = static_cast<SoundManager*>(lua_touserdata(L, lua_upvalueindex(1)));
        const char* name = luaL_checkstring(L, 1);
        mgr->stopSound(name);
        return 0;
        }, 1);
    lua_setfield(L, -2, "stop");

    lua_setglobal(L, "Sound");

    // Aggiunta: funzione globale semplice playSound(nome)
    lua_pushlightuserdata(L, this);
    lua_pushcclosure(L, [](lua_State* L) -> int {
        ScriptManager* manager = static_cast<ScriptManager*>(lua_touserdata(L, lua_upvalueindex(1)));

        if (!lua_isstring(L, 1)) {
            luaL_error(L, "[Lua] playSound richiede almeno un nome di suono come stringa");
            return 0;
        }
        const char* soundName = lua_tostring(L, 1);
        manager->playSound(soundName);
        return 0;
        }, 1);
    lua_setglobal(L, "playSound");
}

std::vector<std::string> ScriptManager::getAvailableEventsForType(const std::string& type) const {
    auto it = availableEvents.find(type);
    if (it != availableEvents.end()) {
        return it->second;
    }
    return {};
}


void ScriptManager::addVariable(const ScriptVariable& variable) {
    if (userVariables.count(variable.name)) {
        std::cerr << "[ScriptManager] Variabile già definita: " << variable.name << "\\n";
        return;
    }

    userVariables[variable.name] = variable;
}

void ScriptManager::addFunction(const ScriptFunction& function) {
    userFunctions[function.name] = function;
}

const std::unordered_map<std::string, ScriptVariable>& ScriptManager::getVariables() const {
    return userVariables;
}

const std::unordered_map<std::string, ScriptFunction>& ScriptManager::getFunctions() const {
    return userFunctions;
}
