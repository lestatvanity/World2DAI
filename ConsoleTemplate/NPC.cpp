#include "NPC.h"
#include "ScriptManager.h"
#include <iostream>

NPC::NPC(const std::string& id, int x, int y)
    : Entity(id, x, y) {
#ifdef CLIENT_BUILD
    texture.loadFromFile("assets/npc.png");
    sprite.setTexture(texture);
    sprite.setPosition(static_cast<float>(x), static_cast<float>(y));
#endif
}

void NPC::update(float dt) {
    if (scriptManager)
        scriptManager->onUpdate(id);

#ifdef CLIENT_BUILD
    sprite.setPosition(static_cast<float>(x), static_cast<float>(y));
#endif
}


const std::string& NPC::getID() const { return id; }
int NPC::getX() const { return x; }
int NPC::getY() const { return y; }

void NPC::setPosition(int newX, int newY) {
    x = newX;
    y = newY;
}

void NPC::moveBy(int dx, int dy) {
    x += dx;
    y += dy;
}

void NPC::say(const std::string& msg) {
    std::cout << "[NPC " << id << "] dice: " << msg << std::endl;
}





#ifdef CLIENT_BUILD
void NPC::draw(sf::RenderTarget& target) const {
    target.draw(sprite);
}
#endif
