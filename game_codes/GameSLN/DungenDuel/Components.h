#ifndef COMPONENTS_H
#define COMPONENTS_H

#include "entity.h"

struct Health : Component {
    int hp;
    Health(int hp = 10) : hp(hp) {}
};

struct Kinematics : Component {
    float x, y, vx, vy, speed;
    Kinematics(float x = 0, float y = 0, float vx = 0, float vy = 0, float speed = 5)
        : x(x), y(y), vx(vx), vy(vy), speed(speed) {}
};

struct Combat : Component {
    float cooldown;
    double last_shot;
    Combat(float cooldown = 0.5) : cooldown(cooldown), last_shot(0) {}
};

struct Armor : Component {
    int value;
    bool is_blocking;
    Armor(int value = 5) : value(value), is_blocking(false) {}
};

#endif