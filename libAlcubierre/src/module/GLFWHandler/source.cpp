#include "module/GLFWHandler/public.h"
#include "module/GLFWHandler/private.h"

#include "core/DebugManager/public.h"

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
        //We construct our Pimpl here because this constructor will not be invoked until registry requests this module.
        PrivatePtr = new GLFWImpl(registry_ptr);
        //Inserting service pointers into WrapperBaseClass unordered map, which is where registry expects the services to be when we promise them in the bundle.
        Services = {{WINDOW_MANAGER, &IWindowManager_GLFWHandler}};
    }

GLFWHandler::~GLFWHandler() {
    if(PrivatePtr) delete PrivatePtr;
}

void* GLFWHandler::GetWindowObjectImpl() {
    return PrivatePtr->Window;
}

bool GLFWHandler::TouchSurfaceApiImpl() {
    return PrivatePtr->TouchSurfaceApi();
}

IWindowManager::WindowInfo* GLFWHandler::GetWindowInfoImpl() {
    return PrivatePtr->GetWindowInfoIMPL();
}

GLFWImpl::GLFWImpl(Registry* registry) : registry_ptr(registry) {
    TouchSurfaceApi();

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

    if(apiStatus) glfwTerminate();
}

#include <cstring>

int GLFWImpl::CreateWindow() {    
    IConfigManager* CM = dynamic_cast<IConfigManager*>(registry_ptr->FetchService(CONFIGURATION_MANAGER));

    std::vector<std::vector<int>> hints = CM->Get<std::vector<std::vector<int>>>("glfw_window_hints", {});
    for(std::vector<int> hint : hints) {
        glfwWindowHint(hint[0], hint[1]);
        DM().Log("Window hint implemented: " + std::to_string(hint[0]) + ", Value: " + std::to_string(hint[1]));
    }

    WindowInfo = new IWindowManager::WindowInfo {};
    WindowInfo->name = CM->Get<std::string>("application_name", "default");
    WindowInfo->width = CM->Get<int>("window_height", 800);
    WindowInfo->height = CM->Get<int>("window_width", 600);
    
    Window = glfwCreateWindow(WindowInfo->height, WindowInfo->width, WindowInfo->name.c_str(), nullptr, nullptr);
    glfwGetFramebufferSize(Window, &WindowInfo->width_pix, &WindowInfo->height_pix);
    
    if(!Window) {
        return 1;
    } else {
        return 0;
    }
}

IWindowManager::WindowInfo* GLFWImpl::GetWindowInfoIMPL() { return WindowInfo; }

bool GLFWImpl::TouchSurfaceApi() {
    if(!apiStatus) {
        glfwInit();
        apiStatus = true;
    }
    return apiStatus; 
}