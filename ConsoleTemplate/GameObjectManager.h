#pragma once
#include <unordered_map>
#include <string>
#include "GameObject.h"

class GameObjectManager {
public:
    void addObject(GameObject* object);
    GameObject* getObject(const std::string& id);
    void removeObject(const std::string& id);
    void clear();

private:
    std::unordered_map<std::string, GameObject*> objects;
};
