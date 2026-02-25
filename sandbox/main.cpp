#include "AlcubierreEngine.h"

#include <fstream>
std::string importDeveloperConfig() {
    std::ifstream settingsFile("settings.json", std::ios::in);
    std::string rawString((std::istreambuf_iterator<char>(settingsFile)), std::istreambuf_iterator<char>());
    settingsFile.close();
    return rawString;
}

std::string importObjFile(std::string path) {
    std::ifstream objFile(path, std::ios::in);
    std::string rawString((std::istreambuf_iterator<char>(objFile)), std::istreambuf_iterator<char>());
    objFile.close();
    return rawString;
}

int main() {
    AlcubierreEngine app;

    app.config.parse(importDeveloperConfig());

    app.initEngine();

    std::string obj = importObjFile("viking_room.obj");
    app.graphics.placeActor({0, 0, 0}, obj);

    while(!app.window.shouldClose()) {
        app.graphics.frame();
    }

    return 0;
}