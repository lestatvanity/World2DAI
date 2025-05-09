#pragma once

#include <SFML/Graphics.hpp>
#include <unordered_map>
#include <string>

class Entity;
extern std::unordered_map<std::string, Entity*> allEntities;

class Entity {
public:
    Entity(const std::string& id, int x, int y);


    virtual void update(float dt);  // per AI, movimento, ecc.


    const std::string& getID() const;
    const std::string& getName() const;
    const std::string& getType() const;
    const std::string& getScript() const;
    bool isActive() const;
    int getX() const;
    int getY() const;

    

    const std::string& getTypeName() const;
    void setTypeName(const std::string& name);


    void setName(const std::string& name);
    void setType(const std::string& type);
    void setScript(const std::string& script);
    void setActive(bool active);
    void setPosition(int x, int y);
    void setX(int x);
    void setY(int y);

    void moveBy(int dx, int dy);
    void updateVisual();
    void draw(sf::RenderTarget& target) const;

    const std::string& getCurrentAnimation() const;
    void setCurrentAnimation(const std::string& animation);
    virtual void render(sf::RenderTarget& target);


private:
    std::string typeName;
    std::string id;
    std::string name;
    std::string type;
    std::string script;
    bool active = true;
    int x, y;
    std::string currentAnimation = "idle";

    sf::Sprite sprite;
};
