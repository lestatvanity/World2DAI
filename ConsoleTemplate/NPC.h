#pragma once
#include "Entity.h"
#include <string>

#ifdef CLIENT_BUILD
#include <SFML/Graphics.hpp>
#endif





class ScriptManager;

class NPC : public Entity{
public:
    NPC(const std::string& id, int x, int y);
    virtual void update(float dt) override;

    const std::string& getID() const;
    int getX() const;
    int getY() const;

    void setPosition(int newX, int newY);
    void moveBy(int dx, int dy);
    void say(const std::string& msg);



#ifdef CLIENT_BUILD
    void draw(sf::RenderTarget& target) const;
#endif

private:
    std::string id;
    int x, y;
    ScriptManager* scriptManager;

#ifdef CLIENT_BUILD
    sf::Texture texture;
    sf::Sprite sprite;
#endif
};
