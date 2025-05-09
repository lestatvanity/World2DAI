#include "WorldController.h"

WorldController::WorldController(GameWorld& world) : world(world) {}

void WorldController::update(float dt) {
    // chiamato ogni frame da Game::run
    world.update(dt);
}

void WorldController::handleInput(const sf::Event& event) {
    if (event.type == sf::Event::KeyPressed) {
        if (event.key.code == sf::Keyboard::W) {
            world.movePlayer("player", 0, -1); // fallback player ID
        }
        if (event.key.code == sf::Keyboard::S) {
            world.movePlayer("player", 0, 1);
        }
        if (event.key.code == sf::Keyboard::A) {
            world.movePlayer("player", -1, 0);
        }
        if (event.key.code == sf::Keyboard::D) {
            world.movePlayer("player", 1, 0);
        }
    }
}
void WorldController::simulate() {
    // server-only logic (tick AI, spawns, script update...)
    world.update(0.016f); // esempio 60fps
}