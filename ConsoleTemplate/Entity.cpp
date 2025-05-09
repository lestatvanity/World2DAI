#include "Entity.h"
#include <unordered_map>
#include <string>

std::unordered_map<std::string, Entity*> allEntities;

Entity::Entity(const std::string& id, int x, int y)
    : id(id), name(id), x(x), y(y), active(true), currentAnimation("idle") {
    sprite.setPosition(static_cast<float>(x), static_cast<float>(y));
}

const std::string& Entity::getID() const { return id; }
const std::string& Entity::getName() const { return name; }
const std::string& Entity::getType() const { return type; }
const std::string& Entity::getScript() const { return script; }
bool Entity::isActive() const { return active; }

int Entity::getX() const { return x; }
int Entity::getY() const { return y; }

void Entity::setName(const std::string& name_) { name = name_; }
void Entity::setType(const std::string& type_) { type = type_; }
void Entity::setScript(const std::string& script_) { script = script_; }
void Entity::setActive(bool a) { active = a; }

void Entity::setPosition(int newX, int newY) {
    x = newX;
    y = newY;
    sprite.setPosition(static_cast<float>(x), static_cast<float>(y));
}

void Entity::setX(int newX) {
    x = newX;
    sprite.setPosition(static_cast<float>(x), static_cast<float>(y));
}

void Entity::setY(int newY) {
    y = newY;
    sprite.setPosition(static_cast<float>(x), static_cast<float>(y));
}

void Entity::moveBy(int dx, int dy) {
    x += dx;
    y += dy;
    sprite.move(static_cast<float>(dx), static_cast<float>(dy));
}

void Entity::updateVisual() {
    sprite.setPosition(static_cast<float>(x), static_cast<float>(y));
}

void Entity::draw(sf::RenderTarget& target) const {
    target.draw(sprite);
}


void Entity::update(float dt) {
    // Per ora: non fa nulla.
    // In futuro potrai mettere logica di AI base, fisica, ecc.
}

void Entity::render(sf::RenderTarget& target) {
    target.draw(sprite);
}



const std::string& Entity::getCurrentAnimation() const { return currentAnimation; }
void Entity::setCurrentAnimation(const std::string& animation) { currentAnimation = animation; }

const std::string& Entity::getTypeName() const { return typeName; }
void Entity::setTypeName(const std::string& name) { typeName = name; }
