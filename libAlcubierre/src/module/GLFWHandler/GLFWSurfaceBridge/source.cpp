#include "module/GLFWHandler/GLFWSurfaceBridge/public.h"
#include "module/GLFWHandler/GLFWSurfaceBridge/private.h"

#include "core/DebugManager/public.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

ModuleRegistryBundle GLFWSurfaceBridge::bundle(
    [](void* registry) -> WrapperBaseClass* { return new GLFWSurfaceBridge(registry); },
    {WINDOW_SURFACE},
    {WINDOW_MANAGER}, //glfwTerminate is delegated to the parent service, which should always be GLFW when this module is being used
    "GLFWSurfaceBridge"
);

GLFWSurfaceBridge::GLFWSurfaceBridge(void* registry) 
    :   IWindowSurfaceBridge_GLFWSurfaceBridge(this),
        registry_ptr(static_cast<Registry*>(registry)) 
    {   
        //We construct our Pimpl here because this constructor will not be invoked until registry requests this module.
        PrivatePtr = new GLFWSurfaceBridgeImpl(registry_ptr);
        //Inserting service pointers into WrapperBaseClass unordered map, which is where registry expects the services to be when we promise them in the bundle.
        Services = {{WINDOW_SURFACE, &IWindowSurfaceBridge_GLFWSurfaceBridge}};
    }

GLFWSurfaceBridge::~GLFWSurfaceBridge() {
    if(PrivatePtr) delete PrivatePtr;
}

int GLFWSurfaceBridge::CreateWindowSurfaceImpl(void* TargetInstance, void* TargetSurfaceObject) {
    return PrivatePtr->createwindowsurface(TargetInstance, TargetSurfaceObject);
}

GLFWSurfaceBridgeImpl::GLFWSurfaceBridgeImpl(Registry* registry) : registry_ptr(registry) {
    glfwInit();

    try {
        IConfigManager* CM = dynamic_cast<IConfigManager*>(registry_ptr->FetchService(CONFIGURATION_MANAGER));

        //GLFW window hint to not create VK window cuz we do that here
            std::vector<std::vector<int>> hints = CM->Get<std::vector<std::vector<int>>>("glfw_window_hints", {});

            hints.push_back({GLFW_CLIENT_API, GLFW_NO_API});

            std::string hintsStr = "[";
            for(int i = 0; i < hints.size(); i++) {
                hintsStr += "[" + std::to_string(hints[i][0]) + "," + std::to_string(hints[i][1]) + "]";
                if(i + 1 < hints.size()) hintsStr += ",";
            }
            hintsStr += "]";

            CM->Set<std::vector<std::vector<int>>>("glfw_window_hints", hintsStr);

            DM().Log("Dumped GLFW-Vulkan bridge hints to cfg");

        //VK extensions needed for GLFW
            std::vector<std::string> extensions = CM->Get<std::vector<std::string>>("extensions", {});

            uint32_t extensionsCount = 0;
            const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&extensionsCount);

            for(uint32_t i = 0; i < extensionsCount; i++) {
                extensions.push_back(glfwExtensions[i]);
            }

            std::string extensionsStr = "[";
            for(int i = 0; i < extensions.size(); i++) {
                extensionsStr += "\"" + extensions[i] + "\"";
                if (i + 1 < extensions.size()) extensionsStr += ",";
            }
            extensionsStr += "]";

            CM->Set<std::vector<std::string>>("extensions", extensionsStr);

            DM().Log("Dumped GLFW-Vulkan bridge extensions to cfg");
    } catch (...) {
        throw;
    }
}

GLFWSurfaceBridgeImpl::~GLFWSurfaceBridgeImpl() {}

int GLFWSurfaceBridgeImpl::createwindowsurface(void* TargetInstance, void* TargetSurfaceObject) {
    IWindowManager* WM = dynamic_cast<IWindowManager*>(registry_ptr->FetchService(WINDOW_MANAGER));

    VkInstance instance = *static_cast<VkInstance*>(TargetInstance);
    GLFWwindow* window = static_cast<GLFWwindow*>(WM->GetWindowObject());
    VkSurfaceKHR* surface = static_cast<VkSurfaceKHR*>(TargetSurfaceObject);

    VkResult hold = glfwCreateWindowSurface(instance, window, nullptr, surface);

    if(hold == VK_SUCCESS) {
        return 1;
    } else {
        DM().Log("Failed to create WindowSurfaceBridge due to VK error: " + std::to_string(hold));
        return 2;
    }
}