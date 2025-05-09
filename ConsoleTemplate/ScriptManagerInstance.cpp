// ScriptManagerInstance.cpp
#include "ScriptManagerInstance.h"

// Inizializzazione del puntatore statico a nullptr
ScriptManager* ScriptManagerInstance::instance = nullptr;

ScriptManager* ScriptManagerInstance::get() {
    return instance;
}

void ScriptManagerInstance::set(ScriptManager* mgr) {
    instance = mgr;
}
