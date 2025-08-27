#include "module/GLFWHandler/public.h"
#include "module/GLFWHandler/private.h"

#include "core/DebugManager/public.h"

#include "core/Registry/interface/IConfigManager.h"

GLFWHandler::GLFWHandler() {
    PrivatePtr = new GLFWImpl();
}

GLFWHandler::~GLFWHandler() {
    delete PrivatePtr;
}

GLFWImpl::GLFWImpl() {
    glfwInit();
    CreateWindow();

    DM().Log("Successfully created GLFW window");
}

GLFWImpl::~GLFWImpl() {
    glfwDestroyWindow(Window);
    glfwTerminate();

    DM().Log("Successfully destroyed GLFW window");
}

#include <cstring>

void GLFWImpl::CreateWindow() {
    IConfigManager* CM = dynamic_cast<IConfigManager*>(Registry::GetRegistry().FetchService("IConfigManager"));

    Window = glfwCreateWindow(CM->Get<int>("window_height", 800), CM->Get<int>("window_width", 600), CM->Get<std::string>("application_name", "default").c_str(), nullptr, nullptr);
    
    if(!Window) {
        glfwTerminate();
        throw std::runtime_error("Failed to create GLFW window");
    } else {
        DM().Log("Successfully constructed GLFW window");
    }
}

ModuleRegistryBundle GLFWHandlerWrapper::bundle(
    []() -> WrapperBaseClass* { return new GLFWHandlerWrapper(); },
    "MODULE_GLFWHANDLER"
);