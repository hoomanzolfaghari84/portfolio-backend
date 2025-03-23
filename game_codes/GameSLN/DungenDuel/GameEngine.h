#ifndef GAME_ENGINE_H
#define GAME_ENGINE_H

#include <unordered_map>
#include <unordered_set>
#include <string>
#include <memory>
#include <thread>
#include <atomic>
#include "entity.h"
#include "systems.h"

#ifdef _WIN32
#ifdef GAME_ENGINE_EXPORTS
#define GAME_ENGINE_API __declspec(dllexport)
#else
#define GAME_ENGINE_API __declspec(dllimport)
#endif
#else
#define GAME_ENGINE_API
#endif

class GameEngine {
    float arena_size = 400;
    float tick_rate = 1.0f / 60.0f;
    std::unordered_map<int, std::unique_ptr<Entity>> entities;
    std::unordered_set<int> players;
    std::atomic<bool> is_running{ false };
    KinematicsSystem kinematics;
    CombatSystem combat;
    ActionSystem action;
    static char* state_json; // To store the JSON state
    std::unique_ptr<std::thread> game_thread;

    void run(); // Private game loop

public:
    GameEngine();
    ~GameEngine();

    GAME_ENGINE_API void start(const int player1_id, const int player2_id);
    GAME_ENGINE_API void end();
    GAME_ENGINE_API void process_input(const int player_id, const char* action_json);
    GAME_ENGINE_API const char* get_state();
    GAME_ENGINE_API bool is_game_over();
};

#endif