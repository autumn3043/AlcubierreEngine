#include "module/GLFWHandler/public.h"
#include "module/GLFWHandler/private.h"

#include "core/DebugManager/public.h"

ModuleRegistryBundle GLFWHandlerWrapper::bundle(
    []() -> WrapperBaseClass* { return new GLFWHandlerWrapper(); },
    "MODULE_GLFWHANDLER"
);

GLFWHandler::GLFWHandler() : IWindowManager_GLFWHandler(this) {}

GLFWHandler::~GLFWHandler() {
    if(PrivatePtr) delete PrivatePtr;
}

int GLFWHandler::WakeImpl() {
    if(!PrivatePtr) {
        PrivatePtr = new GLFWImpl();
        return 1;
    } else {
        return 0;
    }
}

void* GLFWHandler::GetWindowObjectImpl() {
    return PrivatePtr->Window;
}

GLFWImpl::GLFWImpl() {
    glfwInit();

    if(CreateWindow() == 1) {
        throw std::runtime_error("Failed to create GLFW window");
    } else {
        DM().Log("Successfully constructed GLFW window");
    }
}

GLFWImpl::~GLFWImpl() {
    if(Window) {
        glfwDestroyWindow(Window);
        DM().Log("Successfully destroyed GLFW window");
    }

    glfwTerminate();
}

#include <cstring>

int GLFWImpl::CreateWindow() {
    Registry::GetRegistry().FetchService("IWindowSurfaceBridge");
    
    IConfigManager* CM = dynamic_cast<IConfigManager*>(Registry::GetRegistry().FetchService("IConfigManager"));

    std::vector<std::vector<int>> hints = CM->Get<std::vector<std::vector<int>>>("glfw_window_hints", {});
    for(std::vector<int> hint : hints) {
        glfwWindowHint(hint[0], hint[1]);
        DM().Log("Window hint executed: " + std::to_string(hint[0]) + ", Value: " + std::to_string(hint[1]));
    }

    Window = glfwCreateWindow(CM->Get<int>("window_height", 800), CM->Get<int>("window_width", 600), CM->Get<std::string>("application_name", "default").c_str(), nullptr, nullptr);
    
    if(!Window) {
        return 1;
    } else {
        return 0;
    }
}
