#ifndef SYSTEMS_H
#define SYSTEMS_H

#include <unordered_map>
#include <vector>
#include "entity.h"

class KinematicsSystem {
    float arena_size;
public:
    KinematicsSystem(float size = 400) : arena_size(size) {}
    void update(std::unordered_map<int, std::unique_ptr<Entity>>& entities, double delta_time);
};

class CombatSystem {
public:
    void update(std::unordered_map<int, std::unique_ptr<Entity>>& entities, double current_time);
};

class ActionSystem {
    float arena_size;
    int bullet_counter;
public:
    ActionSystem(float size = 400) : arena_size(size), bullet_counter(0) {}
    void process(std::unordered_map<int, std::unique_ptr<Entity>>& entities,
        const int entity_id, const std::string& action_json, double current_time);
};

#endif