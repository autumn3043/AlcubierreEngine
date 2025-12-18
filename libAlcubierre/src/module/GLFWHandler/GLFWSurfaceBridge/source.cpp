#include "module/GLFWHandler/GLFWSurfaceBridge/private.h"

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
        Services = {{WINDOW_SURFACE, &IWindowSurfaceBridge_GLFWSurfaceBridge}};
        init();
    }

GLFWSurfaceBridge::~GLFWSurfaceBridge() {}

int GLFWSurfaceBridge::init() {
    glfwInit();

    try {
        IConfigManager* CM = dynamic_cast<IConfigManager*>(registry_ptr->FetchService(CONFIGURATION_MANAGER));

        //GLFW window hint to not create VK window cuz we do that here
            glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        //VK extensions needed for GLFW
            std::vector<std::string> extensions = CM->Get<std::vector<std::string>>({"renderer", "extensions"}, {});

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

            CM->Set<std::vector<std::string>>({"renderer", "extensions"}, extensionsStr);

            DM().Log("Dumped GLFW-Vulkan bridge extensions to cfg");
    } catch (...) {
        throw;
    }

    return 0;
}

int GLFWSurfaceBridge::createWindowSurface(void* TargetInstance, void* TargetSurfaceObject) {
    IWindowManager* WM = dynamic_cast<IWindowManager*>(registry_ptr->FetchService(WINDOW_MANAGER));

    VkInstance instance = *static_cast<VkInstance*>(TargetInstance);
    GLFWwindow* window = static_cast<GLFWwindow*>(WM->getWindowObject());
    VkSurfaceKHR* surface = static_cast<VkSurfaceKHR*>(TargetSurfaceObject);

    VkResult hold = glfwCreateWindowSurface(instance, window, nullptr, surface);

    if(hold == VK_SUCCESS) {
        return 0;
    } else {
        DM().Log("Failed to create WindowSurfaceBridge due to VK error: " + std::to_string(hold));
        return 1;
    }
}