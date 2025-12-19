#include "AlcubierreEngine.h"

#include <fstream>
std::string importDeveloperConfig() {
    std::ifstream settingsFile("settings.json", std::ios::in);
    std::string rawString((std::istreambuf_iterator<char>(settingsFile)), std::istreambuf_iterator<char>());
    settingsFile.close();
    return rawString;
}

int main() {
    AlcubierreEngine app;

    app.config.parse(importDeveloperConfig());

    app.initEngine();

    while(!app.window.shouldClose()) {
        app.graphics.frame();
    }

    return 0;
}