#include "module/GLFWHandler/public.h"

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
        Init();
    }

GLFWHandler::~GLFWHandler() {
    if(Window) {
        glfwDestroyWindow(Window);
        DM().Log("Successfully destroyed GLFW window");
    }

    if(WindowInfo) delete WindowInfo;

    if(apiStatus) {
        glfwTerminate();
        apiStatus = false;
    }
}

int GLFWHandler::Init() {
    TouchSurfaceApiImpl();
    CreateWindowImpl();
    return 0;
}

void* GLFWHandler::GetWindowObjectImpl() {
    return Window;
}

#include <cstring>

int GLFWHandler::CreateWindowImpl() {    
    IConfigManager* CM = dynamic_cast<IConfigManager*>(registry_ptr->FetchService(CONFIGURATION_MANAGER));

    std::vector<std::vector<int>> hints = CM->Get<std::vector<std::vector<int>>>({"presentation", "window", "hints"}, {});
    for(std::vector<int> hint : hints) {
        glfwWindowHint(hint[0], hint[1]);
        DM().Log("Window hint implemented: " + std::to_string(hint[0]) + ", Value: " + std::to_string(hint[1]));
    }

    if(WindowInfo) delete WindowInfo;
    WindowInfo = new IWindowManager::WindowInfo{};
    WindowInfo->name = CM->Get<std::string>({"metadata", "application_data", "name"}, "default application name");
    WindowInfo->width = CM->Get<int>({"presentation", "window", "width"}, 800);
    WindowInfo->height = CM->Get<int>({"presentation", "window", "height"}, 600);
    
    Window = glfwCreateWindow(WindowInfo->width, WindowInfo->height, WindowInfo->name.c_str(), nullptr, nullptr);
    glfwSetWindowUserPointer(Window, this);
    glfwGetFramebufferSize(Window, &WindowInfo->width_pix, &WindowInfo->height_pix);
    glfwSetFramebufferSizeCallback(Window, framebufferResizeCallback);
    
    if(!Window) {
        throw GLFWException("Failed to create GLFW window");
    } else {
        DM().Log("Constructed GLFW window with name '" + WindowInfo->name + "' width " + std::to_string(WindowInfo->width) + " and height " + std::to_string(WindowInfo->height));
        return 0;
    }
}

IWindowManager::WindowInfo* GLFWHandler::GetWindowInfoImpl() { 
    if(!WindowInfo) {
        DM().Log("Requested window info, but no info was available. Returning blank defaults", 2);
        WindowInfo = new IWindowManager::WindowInfo{};
    }
    return WindowInfo;
}

bool GLFWHandler::TouchSurfaceApiImpl() {
    if(!apiStatus) {
        glfwInit();
        apiStatus = true;
    }
    return apiStatus; 
}

bool GLFWHandler::ShouldCloseImpl() {
    return glfwWindowShouldClose(Window);
}

void GLFWHandler::pollEventsImpl() {
    glfwPollEvents();
}

bool GLFWHandler::suppressFirstResize() {
    if(suppressFirstResizeFlag) {
        suppressFirstResizeFlag = false;
        return true;
    } else {
        return false;
    }
}

bool* GLFWHandler::getFramebufferResizedFlagImpl() {
    return &framebufferResizedFlag;
}

void GLFWHandler::framebufferResizeCallback(GLFWwindow* window, int width, int height) {
    GLFWHandler* handler = reinterpret_cast<GLFWHandler*>(glfwGetWindowUserPointer(window));
    if(handler->suppressFirstResize()) {
        return;
    }
    if(!handler->framebufferResizedFlag) DM().Log("GLFW callback flagged framebuffer resize event");
    handler->framebufferResizedFlag = true;
    glfwGetFramebufferSize(window, &handler->WindowInfo->width_pix, &handler->WindowInfo->height_pix);
}