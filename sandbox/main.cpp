#include "AlcubierreEngine.h"

#include <thread>

int main() {
    AlcubierreEngine app;

    while(!app.ShouldClose()) {
        app.Frame();
        // std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    return 0;
}