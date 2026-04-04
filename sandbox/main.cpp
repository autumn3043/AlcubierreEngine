#include "AlcubierreEngine.h"

#include <fstream>
std::string importDeveloperConfig() {
    std::ifstream settingsFile("settings.json", std::ios::in);
    std::string rawString((std::istreambuf_iterator<char>(settingsFile)), std::istreambuf_iterator<char>());
    settingsFile.close();
    return rawString;
}

std::string importBinaryFile(std::string path) {
    std::ifstream file(path, std::ios::in | std::ios::binary);
    std::string rawString((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();
    return rawString;
}

int main() {
    AlcubierreEngine app;

    app.config.parse(importDeveloperConfig());

    app.initEngine();

    std::string obj = importBinaryFile("assets/meshes/18123728687218786811.blob");
    app.graphics.placeActor({0, 0, 0}, obj);

    while(!app.window.shouldClose()) {
        app.graphics.frame();
    }

    return 0;
}