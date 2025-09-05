#include "module/GLFWHandler/GLFWSurfaceBridge/public.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

ModuleRegistryBundle GLFWSurfaceBridgeWrapper::bundle(
    []() -> WrapperBaseClass* { return new GLFWSurfaceBridgeWrapper(); },
    "MODULE_GLFWHANDLER_GLFWSURFACEBRIDGE"
);

GLFWSurfaceBridge::GLFWSurfaceBridge() : IWindowSurfaceBridge_GLFWSurfaceBridge(this) {}

GLFWSurfaceBridge::~GLFWSurfaceBridge() {}

int GLFWSurfaceBridge::CreateWindowSurfaceImpl(void* TargetInstance, void* TargetSurfaceObject) {
    IWindowManager* WM = dynamic_cast<IWindowManager*>(Registry::GetRegistry().FetchService("IWindowManager"));

    glfwCreateWindowSurface(*static_cast<VkInstance*>(TargetInstance), static_cast<GLFWwindow*>(WM->GetWindowObject()), nullptr, static_cast<VkSurfaceKHR*>(TargetSurfaceObject));

    return 0;
}