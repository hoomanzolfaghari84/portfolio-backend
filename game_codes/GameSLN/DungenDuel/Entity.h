#ifndef ENTITY_H
#define ENTITY_H

#include <string>
#include <unordered_map>
#include <memory>

class Component {
public:
    virtual ~Component() = default;
};

class Entity {
public:
    int id;
    std::unordered_map<std::string, std::unique_ptr<Component>> components;

    Entity(const int id) : id(id) {}
    virtual ~Entity() = default;  // Add virtual destructor
    void addComponent(const std::string& type, std::unique_ptr<Component> comp);
    Component* getComponent(const std::string& type) const;
};


struct Player : Entity {
    Player(const int id, float x, float y);
};

struct Obstacle : Entity {
    float width, height;
    Obstacle(const int id, float x, float y, float w, float h);
};

struct Bullet : Entity {
    Bullet(const int id, float x, float y, float vx, float vy);
};

#endif