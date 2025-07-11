#include "module/GLFWHandler/GLFWHandler.h"

#include "module/DataManager/DataManager.h"
#include "module/DebugManager/DebugManager.h"

#include <cstring>
#include <iostream>
#include <vector>

GLFWHandler::GLFWHandler() {
    CreateWindow(Window);
}

GLFWHandler::~GLFWHandler() {
    glfwDestroyWindow(Window);
    glfwTerminate();

    DebugManager::Log("Successfully destroyed GLFW window");
}

void GLFWHandler::CreateWindow(GLFWwindow*& window) {
    glfwInit();

    // for(DataManagerNamespace::GLFWHint hint : DataManager::GetDataManager().Get("glfw_hints")) {
    //     glfwWindowHint(hint.key, hint.value);
    // }
    DataManager& DM = DataManager::GetDataManager();
    window = glfwCreateWindow(DM.Get<int>("window_width"), DM.Get<int>("window_height"), DM.Get<std::string>("application_name").c_str(), nullptr, nullptr);
    
    if(!window) {
        glfwTerminate();
        throw std::runtime_error("Failed to create GLFW window");
    } else {
        DebugManager::Log("Successfully constructed GLFW window");
    }
}