// ScriptManagerInstance.h
#pragma once
#include "ScriptManager.h"

class ScriptManagerInstance {
public:
    static ScriptManager* get();
    static void set(ScriptManager* mgr);

private:
    static ScriptManager* instance;
};
