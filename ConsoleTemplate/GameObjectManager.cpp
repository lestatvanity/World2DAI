#include "GameObjectManager.h"

void GameObjectManager::addObject(GameObject* object) {
    objects[object->getId()] = object;
}

GameObject* GameObjectManager::getObject(const std::string& id) {
    if (objects.count(id)) {
        return objects[id];
    }
    return nullptr;
}

void GameObjectManager::removeObject(const std::string& id) {
    objects.erase(id);
}

void GameObjectManager::clear() {
    objects.clear();
}
