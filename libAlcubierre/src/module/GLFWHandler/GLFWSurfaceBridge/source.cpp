#include "module/GLFWHandler/GLFWSurfaceBridge/public.h"
#include "module/GLFWHandler/GLFWSurfaceBridge/private.h"

#include "core/DebugManager/public.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

ModuleRegistryBundle GLFWSurfaceBridgeWrapper::bundle(
    []() -> WrapperBaseClass* { return new GLFWSurfaceBridgeWrapper(); },
    "MODULE_GLFWHANDLER_GLFWSURFACEBRIDGE"
);

GLFWSurfaceBridge::GLFWSurfaceBridge() : IWindowSurfaceBridge_GLFWSurfaceBridge(this) {}

GLFWSurfaceBridge::~GLFWSurfaceBridge() {
    if(PrivatePtr) delete PrivatePtr;
}

int GLFWSurfaceBridge::WakeImpl() {
    if(!PrivatePtr) {
        PrivatePtr = new GLFWSurfaceBridgeImpl();
        return 1;
    } else {
        return 0;
    }
}

int GLFWSurfaceBridge::CreateWindowSurfaceImpl(void* TargetInstance, void* TargetSurfaceObject) {
    return PrivatePtr->createwindowsurface(TargetInstance, TargetSurfaceObject);
}

GLFWSurfaceBridgeImpl::GLFWSurfaceBridgeImpl() {
    IConfigManager* CM = dynamic_cast<IConfigManager*>(Registry::GetRegistry().FetchService("IConfigManager"));

    //GLFW window hint to not create VK window cuz we do that here
        std::vector<std::vector<int>> hints = CM->Get<std::vector<std::vector<int>>>("glfw_window_hints", {});

        hints.push_back({GLFW_CLIENT_API, GLFW_NO_API});

        std::string hintsStr = "[";
        for(int i = 0; i < hints.size(); i++) {
            hintsStr += "[" + std::to_string(hints[i][0]) + "," + std::to_string(hints[i][1]) + "]";
            if(i + 1 < hints.size()) hintsStr += ",";
        }
        hintsStr += "]";

        DM().Log("Dumped hints to cfg");

        CM->Set<std::vector<std::vector<int>>>("glfw_window_hints", hintsStr);

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

        DM().Log("Dumped GLFW-Vulkan bridge requirements to cfg");
}

GLFWSurfaceBridgeImpl::~GLFWSurfaceBridgeImpl() {}

int GLFWSurfaceBridgeImpl::createwindowsurface(void* TargetInstance, void* TargetSurfaceObject) {
    IWindowManager* WM = dynamic_cast<IWindowManager*>(Registry::GetRegistry().FetchService("IWindowManager"));

    VkInstance instance = *static_cast<VkInstance*>(TargetInstance);
    GLFWwindow* window = static_cast<GLFWwindow*>(WM->GetWindowObject());
    VkSurfaceKHR* surface = static_cast<VkSurfaceKHR*>(TargetSurfaceObject);

    VkResult hold = glfwCreateWindowSurface(instance, window, nullptr, surface);

    if(hold == VK_SUCCESS) {
        return 0;
    } else {
        DM().Log("Failed to create WindowSurfaceBridge due to VK error: " + std::to_string(hold));
        return 1;
    }
}