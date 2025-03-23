#include "GameEngine.h"
#include "Components.h"
#include <nlohmann/json.hpp>
#include <cmath>
#include <random>
#include <chrono>
#include <cstring> // For strdup

#define GAME_ENGINE_EXPORTS // Define this when building the DLL
using json = nlohmann::json;

static char* last_state = nullptr; // Store last state for get_state()

// Entity method implementations (unchanged)
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

// KinematicsSystem update (unchanged)
void KinematicsSystem::update(std::unordered_map<int, std::unique_ptr<Entity>>& entities, double delta_time) {
    std::vector<std::string> to_remove;
    std::vector<std::pair<std::string, const Obstacle*>> obstacle_list;

    for (const auto& [id, entity] : entities) {
        if (auto* o = dynamic_cast<Obstacle*>(entity.get())) {
            obstacle_list.emplace_back(id, o);
        }
    }

    for (auto& [eid, entity] : entities) {
        auto kin = static_cast<Kinematics*>(entity->getComponent("Kinematics"));
        if (!kin) continue;
        kin->x += kin->vx;
        kin->y += kin->vy;
        kin->x = std::max(0.0f, std::min(kin->x, arena_size - 20));
        kin->y = std::max(0.0f, std::min(kin->y, arena_size - 20));

        if (dynamic_cast<Player*>(entity.get()) || dynamic_cast<Bullet*>(entity.get())) {
            for (const auto& [oid, obstacle] : obstacle_list) {
                if (oid != eid) {
                    auto o_kin = static_cast<Kinematics*>(obstacle->getComponent("Kinematics"));
                    if (kin->x < o_kin->x + obstacle->width &&
                        kin->x + 20 > o_kin->x &&
                        kin->y < o_kin->y + obstacle->height &&
                        kin->y + 20 > o_kin->y) {
                        if (dynamic_cast<Bullet*>(entity.get())) {
                            to_remove.push_back(eid);
                        }
                        else if (dynamic_cast<Player*>(entity.get())) {
                            kin->x -= kin->vx;
                            kin->y -= kin->vy;
                            kin->vx = 0;
                            kin->vy = 0;
                        }
                    }
                }
            }
        }
    }

    for (const auto& eid : to_remove) {
        entities.erase(eid);
    }
}

// CombatSystem update (unchanged)
void CombatSystem::update(std::unordered_map<int, std::unique_ptr<Entity>>& entities, double current_time) {
    std::vector<std::string> to_remove;
    std::vector<std::pair<std::string, Entity*>> bullet_list;
    std::vector<std::pair<std::string, Entity*>> player_list;

    for (auto& [id, entity] : entities) {
        if (dynamic_cast<Bullet*>(entity.get())) {
            bullet_list.emplace_back(id, entity.get());
        }
        else if (dynamic_cast<Player*>(entity.get())) {
            player_list.emplace_back(id, entity.get());
        }
    }

    for (const auto& [bid, bullet] : bullet_list) {
        auto b_kin = static_cast<Kinematics*>(bullet->getComponent("Kinematics"));
        for (const auto& [pid, player] : player_list) {
            if (pid != bid) {
                auto p_kin = static_cast<Kinematics*>(player->getComponent("Kinematics"));
                auto p_health = static_cast<Health*>(player->getComponent("Health"));
                auto p_armor = static_cast<Armor*>(player->getComponent("Armor"));
                if (std::abs(b_kin->x - p_kin->x) < 20 && std::abs(b_kin->y - p_kin->y) < 20) {
                    if (p_armor->is_blocking && p_armor->value > 0) {
                        p_armor->value--;
                    }
                    else {
                        p_health->hp--;
                    }
                    to_remove.push_back(bid);
                    break;
                }
            }
        }
    }

    for (const auto& bid : to_remove) {
        entities.erase(bid);
    }
}

// ActionSystem process (unchanged)
void ActionSystem::process(std::unordered_map<int, std::unique_ptr<Entity>>& entities,
    const int entity_id, const std::string& action_str, double current_time) {
    auto it = entities.find(entity_id);
    if (it == entities.end() || !dynamic_cast<Player*>(it->second.get())) return;

    auto entity = it->second.get();
    auto kin = static_cast<Kinematics*>(entity->getComponent("Kinematics"));
    auto combat = static_cast<Combat*>(entity->getComponent("Combat"));
    auto armor = static_cast<Armor*>(entity->getComponent("Armor"));

    json action = json::parse(action_str);
    std::string type = action["type"];
    if (type == "move") {
        std::string dir = action["direction"];
        if (dir == "up") kin->vy = -kin->speed;
        else if (dir == "down") kin->vy = kin->speed;
        else if (dir == "left") kin->vx = -kin->speed;
        else if (dir == "right") kin->vx = kin->speed;
        else if (dir == "stop_x") kin->vx = 0;
        else if (dir == "stop_y") kin->vy = 0;
    }
    else if (type == "shoot" && current_time - combat->last_shot >= combat->cooldown) {
        combat->last_shot = current_time;
        std::string bullet_id = "bullet_" + std::to_string(bullet_counter++);
        float angle = action["angle"];
        float vx = 10 * std::cos(angle);
        float vy = 10 * std::sin(angle);
        entities[bullet_id] = std::make_unique<Bullet>(bullet_id, kin->x + 10, kin->y + 10, vx, vy);
    }
    else if (type == "block") {
        armor->is_blocking = action["active"];
    }
}

// GameEngine methods
GameEngine::GameEngine() : kinematics(arena_size), action(arena_size) {}

GameEngine::~GameEngine() {
    if (last_state) {
        free(last_state); // Clean up last state
        last_state = nullptr;
    }
}

void GameEngine::start(const char* player1_id, const char* player2_id) {
    players = { player1_id, player2_id };
    entities.clear();
    is_running = true;
    entities[player1_id] = std::make_unique<Player>(player1_id, 50, 150);
    entities[player2_id] = std::make_unique<Player>(player2_id, 350, 150);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(50, 350);
    std::uniform_real_distribution<> size_dis(20, 50);
    for (int i = 0; i < 5; ++i) {
        float x = static_cast<float>(dis(gen));
        float y = static_cast<float>(dis(gen));
        float w = static_cast<float>(size_dis(gen));
        float h = static_cast<float>(size_dis(gen));
        entities["obs_" + std::to_string(i)] = std::make_unique<Obstacle>("obs_" + std::to_string(i), x, y, w, h);
    }
}

void GameEngine::end() {
    is_running = false;
    players.clear();
    entities.clear();
}

void GameEngine::update() {
    if (!is_running) return;
    double current_time = std::chrono::duration_cast<std::chrono::duration<double>>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    kinematics.update(entities);
    combat.update(entities, current_time);

    json state = { {"players", json::array()} };
    for (const auto& player : players) {
        state["players"].push_back(player);
    }
    state_json = state.dump(); // Store JSON as string
}

void GameEngine::process_input(const char* player_id, const char* action_json) {
    if (players.find(player_id) == players.end() || !is_running) return;
    action.process(entities, player_id, action_json, std::chrono::duration_cast<std::chrono::duration<double>>(
        std::chrono::steady_clock::now().time_since_epoch()).count());
}

const char* GameEngine::get_state() {
    json state;
    state["entities"] = json::object();
    state["game_over"] = is_game_over();
    state["is_running"] = is_running;
    state["players"] = json::array();
    for (const auto& pid : players) state["players"].push_back(pid);

    for (const auto& [id, entity] : entities) {
        json e;
        if (auto* p = dynamic_cast<Player*>(entity.get())) {
            e["type"] = "player";
            e["hp"] = static_cast<Health*>(p->getComponent("Health"))->hp;
            e["armor"] = static_cast<Armor*>(p->getComponent("Armor"))->value;
            e["is_blocking"] = static_cast<Armor*>(p->getComponent("Armor"))->is_blocking;
        }
        else if (auto* o = dynamic_cast<Obstacle*>(entity.get())) {
            e["type"] = "obstacle";
            e["width"] = o->width;
            e["height"] = o->height;
        }
        else {
            e["type"] = "bullet";
        }
        auto kin = static_cast<Kinematics*>(entity->getComponent("Kinematics"));
        e["x"] = kin->x;
        e["y"] = kin->y;
        state["entities"][id] = e;
    }

    std::string state_str = state.dump();
    if (last_state) free(last_state); // Free previous state
    last_state = _strdup(state_str.c_str()); // Duplicate for C-style return
    return last_state;
}

bool GameEngine::is_game_over() {
    for (const auto& [id, entity] : entities) {
        if (auto* p = dynamic_cast<Player*>(entity.get())) {
            if (static_cast<Health*>(p->getComponent("Health"))->hp <= 0) return true;
        }
    }
    return false;
}