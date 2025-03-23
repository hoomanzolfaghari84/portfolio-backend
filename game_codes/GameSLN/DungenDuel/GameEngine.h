#ifndef GAME_ENGINE_H
#define GAME_ENGINE_H

#include <unordered_map>
#include <unordered_set>
#include <string>
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
    bool is_running = false;
    KinematicsSystem kinematics;
    CombatSystem combat;
    ActionSystem action;
    static char* last_state; // To store the JSON state

public:
    GameEngine();
    ~GameEngine();

    // Exported methods for Python
    GAME_ENGINE_API void start(const char* player1_id, const char* player2_id);
    GAME_ENGINE_API void end();
    GAME_ENGINE_API void update();
    GAME_ENGINE_API void process_input(const char* player_id, const char* action_json);
    GAME_ENGINE_API const char* get_state(); // Returns a C-string (JSON)
    GAME_ENGINE_API bool is_game_over();
};

#endif