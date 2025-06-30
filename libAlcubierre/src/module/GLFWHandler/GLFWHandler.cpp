#include "module/GLFWHandler/GLFWHandler.h"

#include "module/DataManager/DataManager.h"
#include "module/DebugManager/DebugManager.h"

#include <cstring>
#include <iostream>
#include <vector>

GLFWHandler::GLFWHandler() {
    Window = CreateWindow();
}

GLFWHandler::~GLFWHandler() {
    glfwDestroyWindow(Window);
    glfwTerminate();

    DebugManager::Log("Successfully destroyed GLFW window");
}

GLFWwindow* GLFWHandler::CreateWindow() {
    glfwInit();

    DataManager DATA = DataManager::GetDataManager();

    const DataManagerNamespace::appdata& APPDATA = DATA.GetAppData();
    const DataManagerNamespace::userdata& USERDATA = DATA.GetUserData();

    for(DataManagerNamespace::GLFWHint hint : APPDATA.Hints) {
        glfwWindowHint(hint.key, hint.value);
    }

    GLFWwindow* hold = glfwCreateWindow(USERDATA.WindowWidth, USERDATA.WindowHeight, APPDATA.Name.c_str(), nullptr, nullptr);
    
    if(!hold) {
        glfwTerminate();
        throw std::runtime_error("Failed to create GLFW window");
    } else {
        DebugManager::Log("Successfully constructed GLFW window");
        return hold;
    }
}