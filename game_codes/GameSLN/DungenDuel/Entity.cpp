#include "Entity.h"
#include "Components.h"


// Entity method implementations
void Entity::addComponent(const std::string& type, std::unique_ptr<Component> comp) {
    components[type] = std::move(comp);
}

Component* Entity::getComponent(const std::string& type) const {
    auto it = components.find(type);
    return it != components.end() ? it->second.get() : nullptr;
}

// Player constructor (unchanged)
Player::Player(const int id, float x, float y) : Entity(id) {
    addComponent("Health", std::make_unique<Health>());
    addComponent("Kinematics", std::make_unique<Kinematics>(x, y));
    addComponent("Combat", std::make_unique<Combat>());
    addComponent("Armor", std::make_unique<Armor>());
}

// Obstacle constructor (unchanged)
Obstacle::Obstacle(const int id, float x, float y, float w, float h) : Entity(id), width(w), height(h) {
    addComponent("Kinematics", std::make_unique<Kinematics>(x, y, 0, 0, 0));
}

// Bullet constructor (unchanged)
Bullet::Bullet(const int id, float x, float y, float vx, float vy) : Entity(id) {
    addComponent("Kinematics", std::make_unique<Kinematics>(x, y, vx, vy, 10));
}