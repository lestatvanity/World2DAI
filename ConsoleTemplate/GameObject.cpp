#include "GameObject.h"

GameObject::GameObject(const std::string& id)
    : id(id), openState(false) {}

void GameObject::open() {
    openState = true;
}

void GameObject::close() {
    openState = false;
}

bool GameObject::isOpened() const {
    return openState;
}

std::string GameObject::getId() const {
    return id;
}
