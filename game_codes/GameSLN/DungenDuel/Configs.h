#pragma once
#include <string>

class Configs {
public:
    Configs(const std::string& configFilePath);

    int getWindowWidth() const;
    int getWindowHeight() const;
    bool isFullscreen() const;

private:
    void loadConfig(const std::string& configFilePath);

    int windowWidth = 800; // Default value
    int windowHeight = 600; // Default value
    bool fullscreen = false; // Default value
};
