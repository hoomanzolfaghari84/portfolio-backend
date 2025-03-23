#include "Configs.h"
#include "json/single_include/nlohmann/json.hpp"
#include <fstream>
#include <iostream>


using json = nlohmann::json;

Configs::Configs(const std::string& configFilePath) {
    loadConfig(configFilePath);
}


void Configs::loadConfig(const std::string& configFilePath) {
    std::ifstream configFile(configFilePath);
    if (!configFile.is_open()) {
        std::cerr << "Could not open config file: " << configFilePath << std::endl;
        return;
    }

    json configJson;
    configFile >> configJson;

    // Example of loading configuration parameters
    if (configJson.contains("windowWidth")) {
        windowWidth = configJson["windowWidth"].get<int>();
    }
    if (configJson.contains("windowHeight")) {
        windowHeight = configJson["windowHeight"].get<int>();
    }
    if (configJson.contains("fullscreen")) {
        fullscreen = configJson["fullscreen"].get<bool>();
    }
    // Add more configuration parameters as needed
}

int Configs::getWindowWidth() const {
    return windowWidth;
}

int Configs::getWindowHeight() const {
    return windowHeight;
}

bool Configs::isFullscreen() const {
    return fullscreen;
}