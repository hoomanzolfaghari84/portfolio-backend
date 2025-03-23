#include "Systems.h"
#include "Components.h"


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
