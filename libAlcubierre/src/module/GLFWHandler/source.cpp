#include "module/GLFWHandler/private.h"

ModuleRegistryBundle GLFWHandler::bundle(
    [](void* registry) -> WrapperBaseClass* { return new GLFWHandler(registry); },
    {WINDOW_MANAGER},
    {},
    "GLFWHandler"
);

GLFWHandler::GLFWHandler(void* registry) 
    :   IWindowManager_GLFWHandler(this),
        registry_ptr(static_cast<Registry*>(registry))
    {
        Services = {{WINDOW_MANAGER, &IWindowManager_GLFWHandler}};
        init();
    }

GLFWHandler::~GLFWHandler() {
    if(Window) {
        glfwDestroyWindow(Window);
        DM().Log("Successfully destroyed GLFW window");
    }
    
    glfwTerminate();
}

int GLFWHandler::init() {
    glfwInit();
    createWindow();
    return 0;
}

#include <cstring>

int GLFWHandler::createWindow() {    
    IConfigManager* CM = dynamic_cast<IConfigManager*>(registry_ptr->FetchService(CONFIGURATION_MANAGER));

    if(CM->Get<bool>({"presentation", "window", "permit_resize"}, false)) glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    else glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    if(CM->Get<bool>({"presentation", "window", "maximised"}, true)) glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);
    else glfwWindowHint(GLFW_MAXIMIZED, GLFW_FALSE);

    glfwWindowHint(GLFW_FOCUSED, GLFW_TRUE);
    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);

    std::string name = CM->Get<std::string>({"metadata", "application_data", "name"}, "default application name");
    int width = CM->Get<int>({"presentation", "window", "width"}, 800);
    int height = CM->Get<int>({"presentation", "window", "height"}, 600);
    
    Window = glfwCreateWindow(width, height, name.c_str(), nullptr, nullptr);
    
    if(!Window) {
        throw GLFWException("Failed to create GLFW window");
    } else {
        DM().Log("Constructed GLFW window with name '" + name + "' width " + std::to_string(width) + " and height " + std::to_string(height));
        return 0;
    }
}

void* GLFWHandler::getWindowObject() {
    return Window;
}

IWindowManager::WindowInfo GLFWHandler::getWindowInfo() {
    int width;
    int width_pix;
    int height;
    int height_pix;
    glfwGetWindowSize(Window, &width, &height);
    glfwGetFramebufferSize(Window, &width_pix, &height_pix);
    return { .width = width, .height = height, .width_pix = width_pix, .height_pix = height_pix };
}

void GLFWHandler::pollEvents() {
    glfwPollEvents();
}

bool GLFWHandler::shouldClose() {
    return glfwWindowShouldClose(Window);
}