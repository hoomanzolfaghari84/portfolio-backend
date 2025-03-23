#include "GameEngine.h"
#include "Components.h"
#include <nlohmann/json.hpp>
#include <cmath>
#include <random>
#include <chrono>
#include <cstring>
#include <thread>

using json = nlohmann::json;

char* GameEngine::state_json = nullptr;

GameEngine::GameEngine() : kinematics(arena_size), action(arena_size) {}

GameEngine::~GameEngine() {
    end(); // Ensure thread is stopped
    if (state_json) {
        free(state_json);
        state_json = nullptr;
    }
}

void GameEngine::start(const int player1_id, const int player2_id) {
    if (is_running) return;
    is_running = true;

    players = { player1_id, player2_id };
    entities.clear();
    entities[player1_id] = std::make_unique<Player>(player1_id, 50, 150);
    entities[player2_id] = std::make_unique<Player>(player2_id, 350, 150);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dis(50, 350);
    std::uniform_real_distribution<float> size_dis(20, 50);
    for (int i = 0; i < 5; ++i) {
        entities.emplace("obs_" + std::to_string(i),
            std::make_unique<Obstacle>("obs_" + std::to_string(i), dis(gen), dis(gen), size_dis(gen), size_dis(gen)));
    }

    // Start game loop in a thread
    game_thread = std::make_unique<std::thread>(&GameEngine::run, this);
}

void GameEngine::end() {
    is_running = false;
    if (game_thread && game_thread->joinable()) {
        game_thread->join(); // Wait for thread to finish
        game_thread.reset();
    }
    players.clear();
    entities.clear();
}

void GameEngine::run() {
    auto last_time = std::chrono::steady_clock::now();
    float accumulator = 0.0f;

    while (is_running && !is_game_over()) {
        auto current_time = std::chrono::steady_clock::now();
        float frame_time = std::chrono::duration<float>(current_time - last_time).count();
        last_time = current_time;
        accumulator += frame_time;

        while (accumulator >= tick_rate) {
            double now = std::chrono::duration_cast<std::chrono::duration<double>>(
                std::chrono::steady_clock::now().time_since_epoch()).count();
            kinematics.update(entities, tick_rate);
            combat.update(entities, now);
            accumulator -= tick_rate;
        }

        // Sleep to reduce CPU usage on weak server
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void GameEngine::process_input(const int player_id, const char* action_json) {
    if (!is_running || players.find(player_id) == players.end()) return;
    try {
        double now = std::chrono::duration_cast<std::chrono::duration<double>>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
        action.process(entities, player_id, action_json, now);
    }
    catch (const json::parse_error& e) {
        std::cerr << "Invalid action JSON: " << e.what() << "\n";
    }
}

const char* GameEngine::get_state() {
    json state;
    state["entities"] = json::object();
    state["game_over"] = is_game_over();
    state["is_running"] = is_running.load();
    state["players"] = json::array();
    for (const auto& pid : players) state["players"].push_back(pid);

    if (is_game_over()) {
        for (const auto& pid : players) {
            if (static_cast<Health*>(entities[pid]->getComponent("Health"))->hp > 0) {
                state["winner"] = pid;
                break;
            }
        }
    }

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
        state["entities"][std::to_string(id)] = e;
    }

    std::string state_str = state.dump(-1, ' ', false, json::error_handler_t::replace);
    if (state_json) free(state_json);
    state_json = _strdup(state_str.c_str());
    return state_json;
}

bool GameEngine::is_game_over() {
    int alive_players = 0;
    for (const auto& pid : players) {
        if (static_cast<Health*>(entities[pid]->getComponent("Health"))->hp > 0) {
            alive_players++;
        }
    }
    return alive_players <= 1;
}