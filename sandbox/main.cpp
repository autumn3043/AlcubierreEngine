#include "AlcubierreEngine.h"

#include <fstream>
std::string ImportDeveloperConfig() {
    std::ifstream settingsFile("settings.json", std::ios::in);
    std::string rawString((std::istreambuf_iterator<char>(settingsFile)), std::istreambuf_iterator<char>());
    settingsFile.close();
    return rawString;
}

int main() {
    AlcubierreEngine app;

    app.SetConfigFromJsonString(ImportDeveloperConfig());

    app.InitEngine();

    while(!app.ShouldClose()) {
        app.Frame();
    }

    return 0;
}