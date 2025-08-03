#include "module/GLFWHandler/public.h"
#include "module/GLFWHandler/private.h"

//PROVIDES
#include "core/Registry/interface/IWindowManager.h"

//REQUIRES
#include "core/DebugManager/wrapper.h"
using DM = DebugManager::GetDebugManager();
#include "core/Registry/interface/IConfigManager.h"

GLFWHandler::GLFWHandler() {
    PrivatePtr = new GLFWImpl();

    IWindowManager_IMPL = PrivatePtr->iwindowmanager_impl;
}

GLFWHandler::~GLFWHandler() {
    PrivatePtr->~GLFWHandler();
}

GLFWImpl::GLFWImpl() {
    iwindowmanager_impl = IWindowManager();
    CM = Registry::FetchService("IConfigManager");

    glfwInit();
    CreateWindow();

    DM.Log("Successfully created GLFW window");
}

GLFWImpl::~GLFWImpl() {
    glfwDestroyWindow();
    glfwTerminate();

    DM.Log("Successfully destroyed GLFW window");
}

#include <cstring>

void GLFWImpl::CreateWindow() {
    Window = glfwCreateWindow(CM.Get<int>("window_width"), CM.Get<int>("window_height"), CM.Get<std::string>("application_name").c_str(), nullptr, nullptr);
    
    if(!Window) {
        glfwTerminate();
        throw std::runtime_error("Failed to create GLFW window");
    } else {
        DM.Log("Successfully constructed GLFW window");
    }
}