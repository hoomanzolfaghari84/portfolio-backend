// DungenDuel.cpp : Defines the entry point for the application.
#include "DungenDuel.h"
#include "GameEngine.h"
#include <iostream>

int main() {
    try {
        GameEngine game;
        game.start(1, 2);

        // Simulate running for a while (for testing)
        std::cout << "Game started. Running for 10 seconds...\n";
        for (int i = 0; i < 10; ++i) {
            game.process_input(1, "{\"type\":\"move\",\"direction\":\"right\"}");
            game.process_input(2, "{\"type\":\"shoot\",\"angle\":0}");
            std::cout << "State: " << game.get_state() << "\n";
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        game.end();
        std::cout << "Game ended.\n";
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}