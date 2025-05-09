#pragma once
#include "Entity.h"
#include "imgui_includes.h"
#include <SFML/Graphics.hpp>
#include <string>

class EntityPropertyPanel {
public:
    void setEntity(Entity* entity);
    void update();
    void render(sf::RenderWindow& window);

private:
    Entity* selectedEntity = nullptr;
};
